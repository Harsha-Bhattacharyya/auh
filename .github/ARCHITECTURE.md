# CI/CD Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        auh CI/CD Pipeline                            │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                     CONTINUOUS INTEGRATION                           │
├─────────────────────────────────────────────────────────────────────┤
│  Trigger: Push/PR to main/develop                                   │
│                                                                       │
│  ┌──────────────┐    ┌──────────────┐                              │
│  │   Build &    │    │     Code     │                              │
│  │     Test     │    │   Quality    │                              │
│  ├──────────────┤    ├──────────────┤                              │
│  │ • Build bin  │    │ • clang-tidy │                              │
│  │ • Check fmt  │    │ • lint code  │                              │
│  │ • Build docs │    │              │                              │
│  │ • Upload     │    │              │                              │
│  └──────────────┘    └──────────────┘                              │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                       RELEASE WORKFLOWS                              │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  MAIN RELEASE (Production)                                          │
├─────────────────────────────────────────────────────────────────────┤
│  Trigger: git tag v*.*.*                                            │
│  Tag: v1.0.0, v1.1.0, v2.0.0, etc.                                 │
│  Output: Stable release with changelog                              │
│  Assets: Binary, docs, checksums                                    │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  DEV RELEASE 1: Main Branch (Automatic)                            │
├─────────────────────────────────────────────────────────────────────┤
│  Trigger: Push to main branch                                       │
│  Tag: dev-main (floating)                                           │
│  Version: dev-main-{count}-{sha}                                    │
│  Output: Latest main branch build                                   │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  DEV RELEASE 2: PR Merge (Automatic)                               │
├─────────────────────────────────────────────────────────────────────┤
│  Trigger: PR merged to main                                         │
│  Tag: dev-pr (floating)                                             │
│  Version: dev-pr{number}-{sha}                                      │
│  Output: Post-merge validation build                                │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  DEV RELEASE 3: Nightly (Scheduled)                                │
├─────────────────────────────────────────────────────────────────────┤
│  Trigger: Daily at 2:00 AM UTC (if commits)                        │
│  Tag: dev-nightly (floating)                                        │
│  Version: dev-nightly-{YYYYMMDD}-{sha}                             │
│  Output: Automated nightly build                                    │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  DEV RELEASE 4: Manual Testing (On-Demand)                         │
├─────────────────────────────────────────────────────────────────────┤
│  Trigger: Manual (workflow_dispatch)                                │
│  Tag: dev-{suffix}                                                  │
│  Version: dev-{suffix}-{sha}                                        │
│  Output: Custom testing build                                       │
│  Input: version_suffix, description                                 │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                       WORKFLOW OUTPUTS                               │
└─────────────────────────────────────────────────────────────────────┘

All releases include:
  • auh binary (stripped, optimized)
  • auh.1 (man page)
  • auh.info (info documentation)
  • auh.html (HTML documentation)
  • README.md & COPYING
  • SHA256 checksums
  • Compressed tar.gz package

Main releases:
  • Generated changelog
  • Semantic version tag
  • Production-ready

Dev releases:
  • Pre-release flag
  • Development version
  • Testing purposes
```

## Quick Reference

### Creating a Main Release
```bash
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0
```

### Triggering Dev Release 4 (Manual)
1. Go to GitHub Actions
2. Select "Dev Release 4 - Manual Testing"
3. Click "Run workflow"
4. Enter version suffix (e.g., "beta1", "rc1")
5. Add description
6. Click "Run workflow"

### Installing from Releases
```bash
# Main release
wget https://github.com/Harsha-Bhattacharyya/auh/releases/download/v1.0.0/auh-v1.0.0-linux-x86_64.tar.gz

# Dev release (main branch)
wget https://github.com/Harsha-Bhattacharyya/auh/releases/download/dev-main/auh-dev-main-*-linux-x86_64.tar.gz

# Extract and install
tar -xzf auh-*.tar.gz
sudo install -m 755 auh /usr/bin/
```
