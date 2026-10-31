<div align="center">
  <img src="/assets/kernelsu_next.png" width="96" alt="KernelSU Next Logo">

  <h2>KernelSU Next Standalone SUSFS</h2>
  <p><strong>A kernel-based root solution for Android devices.</strong></p>

  <p>
    <a href="https://github.com/KernelSU-Next/KernelSU-Next/releases/latest">
      <img src="https://img.shields.io/github/v/release/KernelSU-Next/KernelSU-Next?label=Release&logo=github" alt="Latest Release">
    </a>
    <a href="https://nightly.link/KernelSU-Next/KernelSU-Next/workflows/build-manager-ci/next/Manager">
      <img src="https://img.shields.io/badge/Nightly%20Release-gray?logo=hackthebox&logoColor=fff" alt="Nightly Build">
    </a>
    <a href="https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html">
      <img src="https://img.shields.io/badge/License-GPL%20v2-orange.svg?logo=gnu" alt="License: GPL v2">
    </a>
    <a href="/LICENSE">
      <img src="https://img.shields.io/github/license/KernelSU-Next/KernelSU-Next?logo=gnu" alt="GitHub License">
    </a>
    <a title="Crowdin" target="_blank" href="https://crowdin.com/project/kernelsu-next"><img src="https://badges.crowdin.net/kernelsu-next/localized.svg"></a>
  </p>
</div>

**4.19 kernel only.** A KernelSU-Next fork with SUSFS built-in as a standalone module — **no kernel source patching required**.

---

## What is this?

SUSFS (SUS_FS) is a kernel module for hiding modifications from detection. Traditionally, adding SUSFS meant manually patching your kernel source with dozens of hunks across multiple files. This fork integrates SUSFS directly into KernelSU-Next as a self-contained driver under `kernel/susfs/`. You get full SUSFS functionality from a single `curl | bash` setup.

```
curl -LSs https://raw.githubusercontent.com/Youffx/KernelSU-Next/legacy-susfs/kernel/setup.sh | bash -s legacy-susfs
```

To pin a specific release tag instead of tracking the branch:

```
curl -LSs https://raw.githubusercontent.com/Youffx/KernelSU-Next/v3.3.0-legacy-susfs/kernel/setup.sh | bash -s v3.3.0-legacy-susfs
```

---

## Required

As usual, you must manually apply the required hooks to the kernel source, then configure the following options:

```
CONFIG_KSU=y
CONFIG_KSU_MANUAL_HOOK=y
# CONFIG_KSU_KPROBES_HOOK is not set
# CONFIG_KSU_DEBUG is not set
```

Enable SUSFS and its features in your kernel config:

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

The setup script integrates KernelSU-Next into your kernel source tree. All SUSFS code lives in `kernel/susfs/` and is compiled as part of the KernelSU driver — no separate patches, no `fs/stat.c` or `kernel/sys.c` modifications needed.

The build system (`kernel/Kbuild`) auto-detects SUSFS files and wires them in. Just set the config symbols above and build.

---

## SUSFS features

| Feature | Description |
|---------|-------------|
| SUS_MOUNT | Hide suspicious mounts from non-root processes |
| SUS_KSTAT | Spoof file stat (ino, dev, nlink, size, timestamps, blocks) |
| SPOOF_UNAME | Fake kernel release/version in uname |
| OPEN_REDIRECT | Redirect file opens to a decoy path |
| SUS_MAP | Hide suspicious mapped regions |
| HIDE_SYMBOLS | Strip SUSFS symbols from `/proc/kallsyms` |

---

## License

GPL-2.0-only (kernel code).

---

## Credits

[KernelSU-Next](https://github.com/KernelSU-Next/KernelSU-Next) — the upstream project this fork is based on.
