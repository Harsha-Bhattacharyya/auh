# Auh

auh (short for Arch User Helper) is a modern AUR helper.

### Features:
- Tiny codebase for easier modification and bug tracking
- Little Dependencies
- Github mirror install support for AUR DDOS
- Fast compile times
- CLI is like apt for easier use.

### Dependencies:
- `curl`
- `git`
- `jq`
- `base-devel`

### Install:
```bash
git clone https://github.com/Harsha-Bhattacharyya/auh.git
cd auh/
make install
```

### Usage:
  auh <install/installg/remove/update/clean/sync> pkg1 pkg2 ...

  Commands:
  - install: Install packages from AUR (falls back to GitHub mirror if AUR is down)
  - installg: Install packages from GitHub mirror
  - remove: Remove packages
  - update: Update packages or perform full system upgrade
  - clean: Clean package cache
  - sync: List explicitly installed packages that are available in AUR

### LICENSE:
  GPL-V3 located in COPYING

### CONTACT: 
  harshabhattacharyya510@duck.com
