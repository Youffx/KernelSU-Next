<div align="center">
  <img src="/assets/kernelsu_next.png" width="96" alt="KernelSU Next Logo">

  <h2>KernelSU Next Standalone SUSFS</h2>
  <p><strong>A kernel-based root solution for Android devices with built-in SUSFS.</strong></p>

  <p>
    <a href="https://github.com/Youffx/KernelSU-Next/releases/latest">
      <img src="https://img.shields.io/github/v/release/Youffx/KernelSU-Next?label=Release&logo=github" alt="Latest Release">
    </a>
    <a href="https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html">
      <img src="https://img.shields.io/badge/License-GPL%20v2-orange.svg?logo=gnu" alt="License: GPL v2">
    </a>
    <a href="/LICENSE">
      <img src="https://img.shields.io/github/license/Youffx/KernelSU-Next?logo=gnu" alt="GitHub License">
    </a>
  </p>
</div>

---

⚠️ **This fork targets Linux kernel 4.19 only.** It integrates SUSFS as a self-contained driver under `kernel/susfs/` - no manual kernel source patching required.

---

## What is this?

SUSFS (SUS_FS) is a kernel extension for hiding root modifications from detection. Traditionally, adding SUSFS meant manually patching your kernel source with dozens of hunks across multiple files. This fork integrates SUSFS v2.3.0 directly into KernelSU-Next as a standalone module. You get full SUSFS functionality from a single setup command.

```
curl -LSs https://raw.githubusercontent.com/Youffx/KernelSU-Next/legacy-susfs/kernel/setup.sh | bash -s legacy-susfs
```

To pin a specific release tag instead of tracking the branch:

```
curl -LSs https://raw.githubusercontent.com/Youffx/KernelSU-Next/v3.3.0-legacy-susfs/kernel/setup.sh | bash -s v3.3.0-legacy-susfs
```

---

## Requirements

Apply the required KernelSU manual hooks to your kernel source, then configure:

```
CONFIG_KSU=y
CONFIG_KSU_MANUAL_HOOK=y
# CONFIG_KSU_KPROBES_HOOK is not set
# CONFIG_KSU_DEBUG is not set
```

Enable SUSFS and its features:

```
CONFIG_KSU_SUSFS=y
CONFIG_KSU_SUSFS_SUS_MOUNT=y
CONFIG_KSU_SUSFS_SUS_KSTAT=y
CONFIG_KSU_SUSFS_SPOOF_UNAME=y
CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS=y
CONFIG_KSU_SUSFS_OPEN_REDIRECT=y
CONFIG_KSU_SUSFS_SUS_MAP=y
```

---

## How it works

The setup script integrates KernelSU-Next into your kernel source tree. All SUSFS code lives in `kernel/susfs/` and is compiled as part of the KernelSU driver - no separate patches, no modifications to `fs/stat.c`, `kernel/sys.c`, or `fs/proc/cmdline.c` needed.

The build system (`kernel/Kbuild`) auto-injects three hook call sites at build time and declares the function pointers. Just set the config symbols above and build.

---

## SUSFS features

| Feature | Kconfig | Description |
|---------|---------|-------------|
| SUS_MOUNT | `CONFIG_KSU_SUSFS_SUS_MOUNT` | Hide suspicious mounts from non-root processes |
| SUS_KSTAT | `CONFIG_KSU_SUSFS_SUS_KSTAT` | Spoof file stat (ino, dev, nlink, size, timestamps, blocks) |
| SPOOF_UNAME | `CONFIG_KSU_SUSFS_SPOOF_UNAME` | Fake kernel release/version in `uname()` |
| SPOOF_CMDLINE | `CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG` | Hide kernel cmdline from `/proc/cmdline` |
| OPEN_REDIRECT | `CONFIG_KSU_SUSFS_OPEN_REDIRECT` | Redirect file opens to a decoy path |
| SUS_MAP | `CONFIG_KSU_SUSFS_SUS_MAP` | Hide suspicious mapped regions |
| HIDE_SYMBOLS | `CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS` | Strip SUSFS symbols from `/proc/kallsyms` |
| SUS_PATH | `CONFIG_KSU_SUSFS_SUS_PATH` | Hide suspicious paths from directory listings |

---

## Implementation notes for 4.19

- **Hook injection**: `kernel/Kbuild` injects three call sites at build time into `fs/stat.c`, `kernel/sys.c`, and `fs/proc/cmdline.c` with proper `extern` declarations for the hook function pointers.
- **FUSE support**: SUS_PATH and SUS_KSTAT correctly handle FUSE filesystems (e.g., `/sdcard` on Android) by setting flags on both the FUSE inode and its backing inode.
- **Flag storage**: Uses `inode->i_state` bits 33–39 (no collision with 4.19 kernel's `I_*` flags at bits 0–17).
- **TIF flags**: Uses `TIF_SUS_PATH` (33), `TIF_SUS_KSTAT` (34), `TIF_SUS_MAP` (35) - no collision with ARM64 4.19 `TIF_*` bits (0–11, 18–22).
- **Manual hooks only**: Relies on KernelSU's existing LSM hooks (`inode_permission`, `execveat`, `faccessat`, `setresuid`, etc.). Does not use kprobes/kretprobes.

---

## Compatibility

| Kernel version | Status |
|----------------|--------|
| **4.19** | ✅ Tested / primary target |
| Other 4.x | ⚠️ Untested, may need backports |
| 5.x / 6.x (GKI) | ❌ Use upstream KernelSU-Next with LKM instead |

**Architecture:** `arm64`

---

## License

GPL-2.0-only (kernel code).

---

## Credits

- [KernelSU-Next](https://github.com/KernelSU-Next/KernelSU-Next) - upstream project this fork is based on
- [Susfs4ksu](https://gitlab.com/simonpunk/susfs4ksu) - original SUSFS implementation
- [NonGKI_Kernel_Build_2nd](https://github.com/JackA1ltman/NonGKI_Kernel_Build_2nd) - reference 4.19 SUSFS patch