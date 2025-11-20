# CI/CD Pipeline and Release Infrastructure

This document describes the continuous integration and automated release infrastructure for the auh project.

## Overview

The project includes:
- **1 Main Release Workflow**: For stable production releases
- **4 Dev Release Workflows**: For development and testing builds
- **1 CI Workflow**: For continuous integration and quality checks

## Workflows

### CI Workflow (`ci.yml`)

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches
- Manual trigger via workflow_dispatch

**Jobs:**
1. **Build and Test**
   - Installs dependencies
   - Checks code formatting
   - Builds the binary
   - Verifies the binary
   - Builds documentation
   - Uploads artifacts

2. **Code Quality Check**
   - Runs clang-tidy for static analysis

### Main Release Workflow (`release-main.yml`)

**Triggers:**
- Push of version tags matching `v*.*.*` (e.g., `v1.0.0`, `v2.1.3`)

**Process:**
1. Builds optimized binary with stripped symbols
2. Generates documentation
3. Creates release package with checksums
4. Generates changelog from git history
5. Creates GitHub release with assets

**Creating a Main Release:**
```bash
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

### Dev Release 1: Main Branch (`release-dev1-main.yml`)

**Triggers:**
- Push to `main` branch (excluding documentation changes)

**Version Format:** `dev-main-{commit_count}-{short_sha}`

**Purpose:** Provides automatic builds from the main branch for early testing.

### Dev Release 2: PR Merge (`release-dev2-pr.yml`)

**Triggers:**
- When a pull request is merged into `main`

**Version Format:** `dev-pr{number}-{short_sha}`

**Purpose:** Creates builds after PR merges to validate integrated changes.

### Dev Release 3: Nightly (`release-dev3-nightly.yml`)

**Triggers:**
- Scheduled daily at 2:00 AM UTC (only if there are commits in the last 24 hours)
- Manual trigger via workflow_dispatch

**Version Format:** `dev-nightly-{YYYYMMDD}-{short_sha}`

**Purpose:** Provides regular nightly builds for continuous testing.

**Manual Trigger:**
Go to Actions → Dev Release 3 - Nightly → Run workflow

### Dev Release 4: Manual Testing (`release-dev4-manual.yml`)

**Triggers:**
- Manual trigger only via workflow_dispatch

**Version Format:** `dev-{suffix}-{short_sha}`

**Inputs:**
- `version_suffix`: Custom suffix (e.g., beta1, rc1, test1)
- `description`: Release description

**Purpose:** Allows creating custom testing builds on demand.

**Manual Trigger:**
1. Go to Actions → Dev Release 4 - Manual Testing → Run workflow
2. Enter version suffix and description
3. Click "Run workflow"

## Release Assets

Each release includes:
- `auh-{version}-linux-x86_64.tar.gz` - Compressed binary distribution
- `auh-{version}-linux-x86_64.tar.gz.sha256` - SHA256 checksum
- Individual files:
  - `auh` - Binary executable
  - `auh.1` - Man page
  - `auh.info` - Info documentation
  - `auh.html` - HTML documentation

## Installation from Releases

### Main Release
```bash
VERSION=v1.0.0
wget https://github.com/Harsha-Bhattacharyya/auh/releases/download/${VERSION}/auh-${VERSION}-linux-x86_64.tar.gz
tar -xzf auh-${VERSION}-linux-x86_64.tar.gz
sudo install -m 755 auh /usr/bin/
sudo install -m 644 auh.1 /usr/share/man/man1/
```

### Dev Release
```bash
# For main branch builds
wget https://github.com/Harsha-Bhattacharyya/auh/releases/download/dev-main/auh-dev-main-*-linux-x86_64.tar.gz
# Extract and install as above
```

## Workflow Permissions

All release workflows require `contents: write` permission to create releases and manage tags.

## Best Practices

1. **Use main releases for stable versions**: Tag with semantic versioning (v1.0.0, v1.1.0, etc.)
2. **Test with dev releases**: Use dev builds to validate changes before creating main releases
3. **Monitor nightly builds**: Check for regressions in automated nightly builds
4. **Use manual releases for specific testing**: Create custom builds for feature testing or bug verification

## Maintenance

### Updating Dependencies
Edit the workflow files to update the dependency installation commands if new dependencies are added.

### Adjusting Build Process
If the Makefile changes, update the build steps in the workflows accordingly.

### Modifying Release Assets
To include additional files in releases, update the `files:` section in the release workflows.

## Troubleshooting

### Build Failures
1. Check the CI workflow logs for build errors
2. Ensure all dependencies are properly installed
3. Verify the Makefile is correct

### Release Creation Issues
1. Ensure you have proper permissions in the repository
2. Verify the tag format matches the expected pattern
3. Check that the GITHUB_TOKEN has sufficient permissions

### Manual Release Not Appearing
1. Check that you're on the correct branch
2. Verify workflow_dispatch is enabled
3. Check Actions tab for any errors
