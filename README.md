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

#### From Release (Recommended)
```bash
# Download the latest release
VERSION=v1.0.0  # Replace with the latest version
wget https://github.com/Harsha-Bhattacharyya/auh/releases/download/${VERSION}/auh-${VERSION}-linux-x86_64.tar.gz
tar -xzf auh-${VERSION}-linux-x86_64.tar.gz
sudo install -m 755 auh /usr/bin/
sudo install -m 644 auh.1 /usr/share/man/man1/
```

#### From Source
```bash
git clone https://github.com/Harsha-Bhattacharyya/auh.git
cd auh/
make install
```

### Usage:
  auh <command> [options] [packages...]

  Commands:
  - install: Install packages (checks main repos first, then AUR if not found)
  - remove: Remove packages
  - update: Update packages or perform full system upgrade
  - clean: Clean package cache
  - autoremove: Remove orphaned packages (dependencies no longer needed)
  - sync: List explicitly installed packages that are available in AUR

  Install options:
  - -g, --github: Install from GitHub mirror instead of AUR

  Remove options:
  - -s, --autoremove: Also remove dependencies not required by other packages
  - -p, --purge: Also remove configuration files

  Examples:
  - auh install yay pikaur       # Install packages (checks main repos first, then AUR)
  - auh install -g yay           # Install from GitHub mirror
  - auh remove yay               # Remove package only
  - auh remove -s yay            # Remove package with unneeded dependencies (pacman -Rs)
  - auh remove -p yay            # Remove package with config files (pacman -Rn)
  - auh remove -s -p yay         # Remove package with dependencies and configs (pacman -Rns)
  - auh autoremove               # Remove orphaned packages
  - auh update                   # Full system upgrade
  - auh update yay               # Update specific package

### CI/CD and Releases:
  This project includes automated CI/CD pipelines:
  - **Continuous Integration**: Automatic builds and tests on every push and PR
  - **Main Releases**: Stable releases tagged with semantic versioning (v1.0.0, v1.1.0, etc.)
  - **Dev Releases**: Four types of development builds for testing:
    1. Builds from main branch commits
    2. Builds after PR merges
    3. Nightly automated builds
    4. Manual testing builds
  
  See [.github/CICD.md](.github/CICD.md) for detailed documentation.

### LICENSE:
  GPL-V3 located in COPYING

### CONTACT: 
  harshabhattacharyya510@duck.com
