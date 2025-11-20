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
  auh <command> [options] [packages...]

  Commands:
  - install: Install packages from AUR (falls back to GitHub mirror if AUR is down)
  - remove: Remove packages
  - update: Update packages or perform full system upgrade
  - clean: Clean package cache
  - sync: List explicitly installed packages that are available in AUR

  Install options:
  - -g, --github: Install from GitHub mirror instead of AUR

  Remove options:
  - -s, --autoremove: Also remove dependencies not required by other packages

  Examples:
  - auh install yay pikaur       # Install packages from AUR
  - auh install -g yay           # Install from GitHub mirror
  - auh remove yay               # Remove package only
  - auh remove -s yay            # Remove package with dependencies
  - auh update                   # Full system upgrade
  - auh update yay               # Update specific package

### LICENSE:
  GPL-V3 located in COPYING

### CONTACT: 
  harshabhattacharyya510@duck.com
