/*
 * auh - Arch User Helper
 * A modern AUR helper for Arch Linux
 *
 * Copyright (C) 2024 Harsha Bhattacharyya
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Contact: harshabhattacharyya510@duck.com
 */

#include <algorithm>  // For remove, find
#include <array>      // For fixed-size arrays
#include <cstdio>     // For FILE, popen, pclose
#include <cstdlib>    // For system, exit
#include <getopt.h>   // For getopt_long
#include <iostream>   // For cout, cerr
#include <memory>     // For unique_ptr
#include <sstream>    // For istringstream
#include <string>     // For string operations
#include <sys/wait.h> // For wait, WIFEXITED, WEXITSTATUS
#include <unistd.h>   // For fork, pid_t
#include <vector>     // For dynamic arrays

using namespace std;

/**
 * is_valid_package_name - Validate package name format
 * @name: Package name to validate
 *
 * Checks if a package name contains only allowed characters to prevent
 * command injection and ensure compatibility with pacman/AUR standards.
 * Package names should only contain: alphanumeric characters, dash (-),
 * underscore (_), dot (.), and plus (+).
 *
 * Return: true if the package name is valid, false otherwise
 */
static bool
is_valid_package_name (const string &name)
{
  // Package names should only contain alphanumeric, dash, underscore, dot, and
  // plus
  if (name.empty ())
    return false;
  for (char c : name)
    {
      if (!isalnum (c) && c != '-' && c != '_' && c != '.' && c != '+')
        return false;
    }
  return true;
}

/**
 * run_capture - Execute command and capture output
 * @cmd: Shell command to execute
 *
 * Executes a shell command using popen and captures its standard output.
 * Uses a 4KB buffer for reading output. The pipe is automatically closed
 * via unique_ptr with custom deleter.
 *
 * Return: Captured output as string, or empty string on failure
 */
static string
run_capture (const string &cmd)
{
  array<char, 4096> buf;
  string out;
  // Use unique_ptr with pclose as deleter to ensure pipe is closed
  unique_ptr<FILE, decltype (&pclose)> pipe (popen (cmd.c_str (), "r"),
                                             pclose);
  if (!pipe)
    return {};
  // Read output in chunks
  while (fgets (buf.data (), buf.size (), pipe.get ()))
    out += buf.data ();
  return out;
}

/**
 * is_installed - Check if a package is installed
 * @package: Package name to check
 *
 * Uses pacman to query if a package is installed on the system.
 * Redirects output to /dev/null to suppress messages.
 *
 * Return: true if package is installed, false otherwise
 */
static bool
is_installed (const string &package)
{
  string cmd = "pacman -Q " + package + " > /dev/null 2>&1";
  return system (cmd.c_str ()) == 0;
}

/**
 * install_pkg - Install a package from AUR
 * @package: Package name to install
 * @url: Git URL of the package repository
 *
 * Installs a package from the AUR by:
 * 1. Checking if already installed (skip if yes)
 * 2. Querying AUR API to verify package exists
 * 3. Cloning the git repository
 * 4. Building with makepkg
 * 5. Installing the built package
 *
 * Return: 0 on success, 1 on failure
 */
int
install_pkg (const string &package, const string &url)
{
  // Skip if already installed
  if (is_installed (package))
    {
      cout << package << " is already installed; skipping.\n";
      return 0;
    }

  // Query AUR API to check if package exists
  string pcmd = "curl -s \"https://aur.archlinux.org/rpc/?v=5&type=info&arg="
                + package + "\" | jq -c .results";
  string out = run_capture (pcmd);
  // Trim trailing whitespace
  while (!out.empty () && isspace ((unsigned char)out.back ()))
    out.pop_back ();

  // Empty results array means package not found
  if (out == "[]")
    {
      cerr << "Package not found: " << package << '\n';
      return 1;
    }

  // Clone the package repository
  cout << "Cloning " << package << "...\n";
  string ccmd = "git clone " + url + " &> /dev/null";
  if (system (ccmd.c_str ()) != 0)
    {
      cerr << "git clone failed for " << package << '\n';
      return 1;
    }

  // Build and install the package
  cout << "Building " << package << "...\n";
  string mcmd = "cd " + package + " && makepkg -si --noconfirm";
  if (system (mcmd.c_str ()) != 0)
    {
      cerr << "makepkg failed for " << package << '\n';
      return 1;
    }

  return 0;
}

/**
 * remove_pkg - Remove an installed package
 * @package: Package name to remove
 * @autoremove: If true, also remove dependencies not required by other packages
 *
 * Removes a package using pacman:
 * -R: Remove package
 * -s: Remove dependencies not required by other packages (if autoremove is true)
 * -n: Remove configuration files (if autoremove is true)
 *
 * Return: 0 on success, 1 on failure
 */
int
remove_pkg (const string &package, bool autoremove = false)
{
  // Check if package is installed before attempting removal
  if (!is_installed (package))
    {
      cout << package << " is not installed; skipping removal.\n";
      return 0;
    }
  
  string flags = autoremove ? "-Rsn" : "-R";
  string cmd = "sudo pacman " + flags + " --noconfirm " + package;
  cout << "Removing " << package << "...\n";
  int rc = system (cmd.c_str ());
  if (rc != 0)
    {
      cerr << "Removal failed for " << package << " (code " << rc << ")\n";
      return 1;
    }
  return 0;
}

/**
 * update_pkg - Update a package or perform system upgrade
 * @package: Package name to update, or empty string for full system upgrade
 *
 * If package is empty: performs full system upgrade using pacman -Syu
 * If package is specified: attempts to update via pacman first, then falls
 * back to rebuilding from AUR if needed.
 *
 * For AUR packages, the function:
 * 1. Creates a temporary directory in /tmp
 * 2. Clones the latest version from AUR
 * 3. Rebuilds and installs the package
 * 4. Cleans up the temporary directory
 *
 * Return: 0 on success, 1 on failure
 */
int
update_pkg (const string &package)
{
  if (package.empty ())
    {
      // Full system upgrade
      cout << "Performing full system upgrade...\n";
      int rc = system ("sudo pacman -Syu --noconfirm");
      if (rc != 0)
        {
          cerr << "System update failed (code " << rc << ")\n";
          return 1;
        }
      return 0;
    }
  else
    {
      // Update single package via pacman if available in repos;
      // for AUR packages, rebuild using makepkg
      if (is_installed (package))
        {
          cout << "Updating repo package " << package << "...\n";
          int rc = system (("sudo pacman -S --noconfirm " + package).c_str ());
          if (rc == 0)
            return 0;
          // Fall back to AUR rebuild if pacman update fails
        }
      
      // Rebuild from AUR
      cout << "Rebuilding AUR package " << package << "...\n";
      string url = "https://aur.archlinux.org/" + package + ".git";
      string tmpdir = "/tmp/auh_" + package;
      
      // Clean old directory, clone fresh copy, build and install
      int rc = system (("rm -rf " + tmpdir + " && git clone " + url + " "
                        + tmpdir + " &> /dev/null")
                           .c_str ());
      if (rc != 0)
        {
          cerr << "Failed to clone AUR for " << package << '\n';
          return 1;
        }
      
      rc = system (("cd " + tmpdir + " && makepkg -si --noconfirm").c_str ());
      if (rc != 0)
        {
          cerr << "Rebuild/install failed for " << package << '\n';
          return 1;
        }
      
      // Clean up temporary directory
      system (("rm -rf " + tmpdir).c_str ());
      return 0;
    }
}

/**
 * clean_cache - Clean the pacman package cache
 *
 * Runs 'pacman -Scc' to clean both the package cache and unused sync databases.
 * This frees up disk space by removing downloaded package files.
 *
 * Return: 0 on success, 1 on failure
 */
int
clean_cache ()
{
  string cmd = "sudo pacman -Scc --noconfirm";
  int rc = system (cmd.c_str ());
  if (rc == 0)
    {
      cout << "Successfully cleaned\n";
      return 0;
    }
  else
    {
      cout << "System cleaning failed\n";
      return 1;
    }
}

/**
 * build_from_github - Build and install package from GitHub mirror
 * @package: Package name to install
 * @mirror_url_base: Base URL of the GitHub mirror (default: archlinux/aur)
 *
 * Installs a package from GitHub mirror instead of AUR. Useful when:
 * - AUR is experiencing downtime or DDOS
 * - Mirror is preferred for reliability
 * - Testing mirror functionality
 *
 * The function:
 * 1. Creates a temporary working directory
 * 2. Clones the package from GitHub mirror (shallow clone, single branch)
 * 3. Builds the package using makepkg with --skippgpcheck
 * 4. Installs the built package
 * 5. Cleans up the temporary directory
 *
 * Return: 0 on success, 1 on clone failure, 4 on build failure
 */
int
build_from_github (const std::string &package,
                   const std::string &mirror_url_base
                   = "https://github.com/archlinux/aur")
{
  // Create temporary working directory
  const std::string tmpdir = "./auh_mirror_" + package;
  
  // Ensure clean temporary directory
  std::string rm = "rm -rf " + tmpdir;
  system (rm.c_str ());

  // Clone mirror with shallow clone for speed (single branch, depth=1, no tags)
  std::string clone_cmd = "git clone --single-branch --branch " + package
                          + " --depth=1 " + mirror_url_base + ".git " + tmpdir
                          + " 2>/dev/null";
  if (system (clone_cmd.c_str ()) != 0)
    {
      std::cerr << "Failed to clone mirror for " << package << '\n';
      return 1;
    }

  // Build and install package (skip PGP checks for mirror packages)
  std::string mkcmd
      = "cd " + tmpdir + " && makepkg -si --noconfirm --skippgpcheck";
  int mkrc = system (mkcmd.c_str ());

  // Clean up temporary directory
  system (("rm -rf " + tmpdir).c_str ());

  if (mkrc != 0)
    {
      std::cerr << "makepkg failed for " << package << " (code " << mkrc
                << ")\n";
      return 4;
    }

  std::cout << "Built and installed " << package << " from mirror branch.\n";
  return 0;
}

/**
 * is_aur_up - Check if AUR is accessible
 *
 * Tests AUR availability by sending an HTTP request and checking the status code.
 * Uses curl to get the HTTP response code from the AUR website.
 *
 * Return: true if AUR is up (HTTP 200-399), false otherwise
 */
bool
is_aur_up ()
{
  const std::string url = "https://aur.archlinux.org";

  // Get HTTP status code using curl
  std::string curl_cmd = "curl -s -o /dev/null -w \"%{http_code}\" " + url;
  std::string http_code_str = run_capture (curl_cmd);

  int http_code = 0;
  try
    {
      http_code = std::stoi (http_code_str);
    }
  catch (...)
    {
      std::cerr << "Failed to get AUR HTTP status code\n";
      return 1;
    }

  // HTTP codes 200-399 indicate success/redirection (AUR is up)
  std::string status_text;
  if (http_code >= 200 && http_code < 400)
    {
      return true;
    }
  else
    {
      return false;
    }
}

/**
 * install_packages_parallel - Install multiple packages in parallel
 * @packages: Vector of package names to install
 * @use_aur: If true, install from AUR; if false, use GitHub mirror
 *
 * Installs multiple packages in parallel using fork() to improve performance.
 * The function limits concurrent installations to prevent system overload.
 *
 * Process flow:
 * 1. Validates each package name before processing
 * 2. Forks child processes (up to max_concurrent limit)
 * 3. Each child installs one package
 * 4. Parent waits for children to complete
 * 5. Tracks failures and reports summary
 *
 * The parallel installation can significantly reduce total installation time
 * when installing multiple packages, especially for packages with no
 * interdependencies.
 *
 * Return: 0 if all packages installed successfully, 1 if any failed
 */
int
install_packages_parallel (const vector<string> &packages, bool use_aur)
{
  // Limit concurrent installations to prevent overwhelming the system
  const size_t max_concurrent = 4;
  vector<pid_t> children;
  size_t pkg_idx = 0;
  int failed_count = 0;

  // Process packages: start new installations and wait for completions
  while (pkg_idx < packages.size () || !children.empty ())
    {
      // Start new processes up to the concurrency limit
      while (pkg_idx < packages.size () && children.size () < max_concurrent)
        {
          const string &pkg = packages[pkg_idx];

          // Validate package name before processing to prevent injection attacks
          if (!is_valid_package_name (pkg))
            {
              cerr << "Invalid package name: " << pkg << '\n';
              failed_count++;
              pkg_idx++;
              continue;
            }

          pid_t pid = fork ();

          if (pid == 0)
            {
              // Child process: install the package and exit with result code
              string url = "https://aur.archlinux.org/" + pkg + ".git";
              int result;
              if (use_aur)
                result = install_pkg (pkg, url);
              else
                result = build_from_github (pkg);
              exit (result);
            }
          else if (pid > 0)
            {
              // Parent process: store child PID for tracking
              children.push_back (pid);
              pkg_idx++;
            }
          else
            {
              // Fork failed
              cerr << "Failed to fork for package: " << pkg << '\n';
              failed_count++;
              pkg_idx++;
            }
        }

      // Wait for at least one child to complete before starting more
      if (!children.empty ())
        {
          int status;
          pid_t finished = wait (&status);
          if (finished > 0)
            {
              // Remove finished process from children vector
              children.erase (
                  remove (children.begin (), children.end (), finished),
                  children.end ());

              // Check exit status and track failures
              if (WIFEXITED (status) && WEXITSTATUS (status) != 0)
                {
                  failed_count++;
                }
            }
        }
    }

  // Report summary if there were failures
  if (failed_count > 0)
    {
      cerr << failed_count << " package(s) failed to install.\n";
      return 1;
    }

  return 0;
}

/**
 * sync_explicit - List explicitly installed AUR packages
 *
 * Queries pacman for all explicitly installed packages (-Qe), then checks
 * each one against the AUR API to determine which are from AUR.
 *
 * This is useful for:
 * - Auditing which packages came from AUR
 * - Managing your AUR package list
 * - Identifying packages for backup/migration
 *
 * The function validates package names and uses the AUR RPC API to check
 * package existence.
 *
 * Return: 0 on success
 */
int
sync_explicit ()
{
  // Get list of explicitly installed packages
  string cmd = "pacman -Qeq";
  string explicit_pkgs = run_capture (cmd);

  if (explicit_pkgs.empty ())
    {
      cout << "No explicitly installed packages found.\n";
      return 0;
    }

  cout << "Checking explicitly installed packages against AUR...\n";

  // Parse package list using stringstream for cleaner line-by-line parsing
  istringstream stream (explicit_pkgs);
  string pkg;
  int synced_count = 0;

  while (getline (stream, pkg))
    {
      // Trim trailing whitespace from package name
      while (!pkg.empty () && isspace ((unsigned char)pkg.back ()))
        pkg.pop_back ();

      if (pkg.empty ())
        continue;

      // Validate package name format
      if (!is_valid_package_name (pkg))
        {
          cerr << "Skipping invalid package name: " << pkg << '\n';
          continue;
        }

      // Query AUR API to check if package exists
      string pcmd = "curl -s "
                    "\"https://aur.archlinux.org/rpc/?v=5&type=info&arg="
                    + pkg + "\" | jq -r '.results | length'";
      string result = run_capture (pcmd);

      // Trim whitespace from result
      while (!result.empty () && isspace ((unsigned char)result.back ()))
        result.pop_back ();

      // If result is "1", package exists in AUR
      if (result == "1")
        {
          cout << "Found AUR package: " << pkg << "\n";
          synced_count++;
        }
    }

  cout << "Total AUR packages found in explicitly installed: " << synced_count
       << "\n";
  return 0;
}

/**
 * print_usage - Display program usage information
 *
 * Prints the command-line usage syntax and available commands for auh.
 */
void
print_usage ()
{
  cout << "Usage: auh <command> [options] [packages...]\n\n";
  cout << "Commands:\n";
  cout << "  install     Install packages from AUR\n";
  cout << "  remove      Remove packages\n";
  cout << "  update      Update packages or perform full system upgrade\n";
  cout << "  clean       Clean package cache\n";
  cout << "  sync        List explicitly installed AUR packages\n\n";
  cout << "Install options:\n";
  cout << "  -g, --github    Install from GitHub mirror instead of AUR\n\n";
  cout << "Remove options:\n";
  cout << "  -s, --autoremove    Also remove dependencies not required by other packages\n\n";
  cout << "Examples:\n";
  cout << "  auh install yay pikaur       # Install packages from AUR\n";
  cout << "  auh install -g yay           # Install from GitHub mirror\n";
  cout << "  auh remove yay               # Remove package only\n";
  cout << "  auh remove -s yay            # Remove package with dependencies\n";
  cout << "  auh update                   # Full system upgrade\n";
  cout << "  auh update yay               # Update specific package\n";
}

/**
 * main - Entry point for auh program
 * @argc: Argument count
 * @argv: Argument vector
 *
 * Parses command-line arguments using getopt_long and dispatches to appropriate
 * handler functions.
 *
 * Supported commands:
 * - install: Install packages from AUR (supports -g/--github flag)
 * - remove: Remove packages (supports -s/--autoremove flag)
 * - update: Update packages or perform full system upgrade
 * - clean: Clean package cache
 * - sync: List explicitly installed AUR packages
 *
 * Return: 0 on success, 1 on error or invalid command
 */
int
main (int argc, char **argv)
{
  // Require at least one command argument
  if (argc < 2)
    {
      print_usage ();
      return 1;
    }

  string cmd = argv[1];

  if (cmd == "install")
    {
      // Parse install options
      bool use_github = false;
      int opt;
      
      // Define long options for install command
      static struct option long_options[] = {
        {"github", no_argument, 0, 'g'},
        {0, 0, 0, 0}
      };
      
      // Reset getopt state for proper parsing
      optind = 2;
      
      // Parse options
      while ((opt = getopt_long (argc, argv, "g", long_options, NULL)) != -1)
        {
          switch (opt)
            {
            case 'g':
              use_github = true;
              break;
            default:
              cout << "Usage: auh install [-g|--github] <packages...>\n";
              return 1;
            }
        }
      
      // Check if packages are provided
      if (optind >= argc)
        {
          cout << "Usage: auh install [-g|--github] <packages...>\n";
          return 1;
        }
      
      // Collect all packages into a vector
      vector<string> packages;
      for (int i = optind; i < argc; ++i)
        {
          packages.push_back (argv[i]);
        }
      
      // Determine whether to use AUR or GitHub
      bool use_aur = !use_github;
      if (use_aur)
        {
          // Check AUR availability for automatic fallback
          use_aur = is_aur_up ();
        }
      
      // Install packages in parallel
      return install_packages_parallel (packages, use_aur);
    }
  else if (cmd == "remove")
    {
      // Parse remove options
      bool autoremove = false;
      int opt;
      
      // Define long options for remove command
      static struct option long_options[] = {
        {"autoremove", no_argument, 0, 's'},
        {0, 0, 0, 0}
      };
      
      // Reset getopt state for proper parsing
      optind = 2;
      
      // Parse options
      while ((opt = getopt_long (argc, argv, "s", long_options, NULL)) != -1)
        {
          switch (opt)
            {
            case 's':
              autoremove = true;
              break;
            default:
              cout << "Usage: auh remove [-s|--autoremove] <packages...>\n";
              return 1;
            }
        }
      
      // Check if packages are provided
      if (optind >= argc)
        {
          cout << "Usage: auh remove [-s|--autoremove] <packages...>\n";
          return 1;
        }
      
      // Remove each package sequentially
      for (int i = optind; i < argc; ++i)
        remove_pkg (argv[i], autoremove);
    }
  else if (cmd == "update")
    {
      if (argc == 2)
        {
          // No package specified: full system update
          update_pkg ("");
        }
      else
        {
          // Update specified packages
          for (int i = 2; i < argc; ++i)
            update_pkg (argv[i]);
        }
    }
  else if (cmd == "clean")
    {
      // Clean package cache
      clean_cache ();
    }
  else if (cmd == "sync")
    {
      // List explicitly installed AUR packages
      sync_explicit ();
    }
  else
    {
      // Unknown command
      cout << "Unknown command: " << cmd << "\n\n";
      print_usage ();
      return 1;
    }

  return 0;
}
