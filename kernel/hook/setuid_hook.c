#include <linux/compiler.h>
#include <linux/version.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/task_work.h>
#include <linux/thread_info.h>
#include <linux/seccomp.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>

#include "policy/allowlist.h"
#include "hook/setuid_hook.h"
#include "klog.h" // IWYU pragma: keep
#include "manager/manager_identity.h"
#include "infra/seccomp_cache.h"
#include "supercall/supercall.h"
#include "hook/hook_manager.h"
#include "feature/kernel_umount.h"
#include "compat/kernel_compat.h"
#ifdef CONFIG_KSU_SUSFS
#include "../susfs/susfs_def.h"
#endif // #ifdef CONFIG_KSU_SUSFS

extern void disable_seccomp(void);

static inline bool is_zygote_isolated_service_uid(uid_t uid)
{
    uid %= 100000;
    return (uid >= 99000 && uid < 100000);
}

static inline bool is_zygote_normal_app_uid(uid_t uid)
{
    uid %= 100000;
    return (uid >= 10000 && uid < 19999);
}

#ifdef CONFIG_KSU_SUSFS
extern u32 susfs_zygote_sid;
extern struct cred *ksu_cred;

#ifdef CONFIG_KSU_SUSFS_SUS_PATH
extern void susfs_run_sus_path_loop(void);
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_PATH

struct susfs_handle_setuid_tw {
    struct callback_head cb;
};

static void susfs_handle_setuid_tw_func(struct callback_head *cb)
{
    struct susfs_handle_setuid_tw *tw = container_of(cb, struct susfs_handle_setuid_tw, cb);
    const struct cred *saved = override_creds(ksu_cred);

#ifdef CONFIG_KSU_SUSFS_SUS_PATH
    susfs_run_sus_path_loop();
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_PATH

    revert_creds(saved);
    kfree(tw);
}

static void ksu_handle_extra_susfs_work(void)
{
    struct susfs_handle_setuid_tw *tw = kzalloc(sizeof(*tw), GFP_ATOMIC);

    if (!tw) {
        pr_err("susfs: No enough memory\n");
        return;
    }

    tw->cb.func = susfs_handle_setuid_tw_func;

    int err = task_work_add(current, &tw->cb, TWA_RESUME);
    if (err) {
        kfree(tw);
        pr_err("susfs: Failed adding task_work 'susfs_handle_setuid_tw', err: %d\n", err);
    }
}
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
extern void susfs_try_umount(uid_t uid);
#endif // #ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
#endif // #ifdef CONFIG_KSU_SUSFS

int ksu_handle_setresuid(uid_t old_uid, uid_t new_uid)
{
    // we rely on the fact that zygote always call setresuid(3) with same uids

    // We only interest in process spwaned by zygote
#ifdef CONFIG_KSU_SUSFS
    if (!susfs_is_sid_equal(current_cred(), susfs_zygote_sid)) {
        return 0;
    }
#endif

#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
    // Check if spawned process is isolated service first, and force to do umount if so  
    if (is_zygote_isolated_service_uid(new_uid)) {
        goto do_umount;
    }
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_MOUNT

    pr_debug("handle_setresuid from %d to %d\n", old_uid, new_uid);

    if (unlikely(is_uid_manager(new_uid))) {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
        if (current->seccomp.mode == SECCOMP_MODE_FILTER && current->seccomp.filter) {
            ksu_seccomp_allow_cache(current->seccomp.filter, __NR_reboot);
        }
#endif

#ifdef KSU_KPROBES_HOOK
        ksu_set_task_tracepoint_flag(current);
#endif

        pr_info("install fd for manager: %d\n", new_uid);
        ksu_install_fd();
        return 0;
    }

// Check if spawned process is normal user app and needs to be umounted
    if (likely(is_zygote_normal_app_uid(new_uid) && ksu_uid_should_umount(new_uid))) {
        goto do_umount;
    }

	if (ksu_is_allow_uid_for_current(new_uid)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
        if (current->seccomp.mode == SECCOMP_MODE_FILTER && current->seccomp.filter) {
            ksu_seccomp_allow_cache(current->seccomp.filter, __NR_reboot);
        }
#else
		disable_seccomp();
#endif

#ifdef KSU_KPROBES_HOOK
		ksu_set_task_tracepoint_flag(current);
#endif
	} else {
#ifdef KSU_KPROBES_HOOK
		ksu_clear_task_tracepoint_flag_if_needed(current);
#endif
    }

    // Handle kernel umount
    //ksu_handle_umount(old_uid, new_uid);

    return 0;

do_umount:
    // Handle kernel umount
#ifndef CONFIG_KSU_SUSFS_TRY_UMOUNT
    ksu_handle_umount(old_uid, new_uid);
#else
    susfs_try_umount(new_uid);
#endif // #ifndef CONFIG_KSU_SUSFS_TRY_UMOUNT

#ifdef CONFIG_KSU_SUSFS_SUS_PATH
    //susfs_run_sus_path_loop(new_uid);
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_PATH

#ifdef CONFIG_KSU_SUSFS
    ksu_handle_extra_susfs_work();
    susfs_set_current_proc_umounted();
#endif

    return 0;
}

void __init ksu_setuid_hook_init(void)
{
	ksu_kernel_umount_init();
}

void __exit ksu_setuid_hook_exit(void)
{
	pr_info("ksu_core_exit\n");
	ksu_kernel_umount_exit();
}