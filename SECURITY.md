# Security Policy

## Supported Versions

We release patches for security vulnerabilities. Currently supported versions:

| Version | Supported          |
| ------- | ------------------ |
| latest  | :white_check_mark: |

## Reporting a Vulnerability

The auh team takes security bugs seriously. We appreciate your efforts to responsibly disclose your findings.

### How to Report a Security Vulnerability

**Please do not report security vulnerabilities through public GitHub issues.**

Instead, please report them via email to: harshabhattacharyya510@duck.com

You should receive a response within 48 hours. If for some reason you do not, please follow up via email to ensure we received your original message.

Please include the following information in your report:

- Type of issue (e.g., buffer overflow, SQL injection, cross-site scripting, etc.)
- Full paths of source file(s) related to the manifestation of the issue
- The location of the affected source code (tag/branch/commit or direct URL)
- Any special configuration required to reproduce the issue
- Step-by-step instructions to reproduce the issue
- Proof-of-concept or exploit code (if possible)
- Impact of the issue, including how an attacker might exploit the issue

This information will help us triage your report more quickly.

### What to Expect

- We will acknowledge receipt of your vulnerability report within 48 hours
- We will send you regular updates about our progress
- If you have followed the instructions above, we will not take any legal action against you regarding the report
- We will handle your report with strict confidentiality, and not pass on your personal details to third parties without your permission
- We will keep you informed of the progress towards resolving the problem
- We will credit you in the security advisory (unless you prefer to remain anonymous)

## Security Best Practices

When using auh:

1. **Package Verification**: Always verify packages before installation
2. **Keep Updated**: Keep auh and your system up to date
3. **Review PKGBUILDs**: Inspect PKGBUILD files before building packages
4. **Use Official Sources**: Prefer official repositories when available
5. **Check Dependencies**: Review package dependencies before installation
6. **Minimal Privileges**: Don't run auh with unnecessary elevated privileges (it will request sudo when needed)

## Known Security Considerations

- auh executes shell commands and PKGBUILDs which can run arbitrary code
- Always review packages from the AUR before installing them
- The tool requires sudo access for certain operations (package installation/removal)
- Network operations use HTTPS by default for secure communication

## Disclosure Policy

When we receive a security bug report, we will:

1. Confirm the problem and determine affected versions
2. Audit code to find any similar problems
3. Prepare fixes for all supported versions
4. Release new versions as soon as possible

## Comments on this Policy

If you have suggestions on how this process could be improved, please submit a pull request.
