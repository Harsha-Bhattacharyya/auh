#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

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

static string
run_capture (const string &cmd)
{
  array<char, 4096> buf;
  string out;
  unique_ptr<FILE, decltype (&pclose)> pipe (popen (cmd.c_str (), "r"),
                                             pclose);
  if (!pipe)
    return {};
  while (fgets (buf.data (), buf.size (), pipe.get ()))
    out += buf.data ();
  return out;
}

static bool
is_installed (const string &package)
{
  string cmd = "pacman -Q " + package + " > /dev/null 2>&1";
  return system (cmd.c_str ()) == 0;
}

int
install_pkg (const string &package, const string &url)
{
  if (is_installed (package))
    {
      cout << package << " is already installed; skipping.\n";
      return 0;
    }

  string pcmd = "curl -s \"https://aur.archlinux.org/rpc/?v=5&type=info&arg="
                + package + "\" | jq -c .results";
  string out = run_capture (pcmd);
  while (!out.empty () && isspace ((unsigned char)out.back ()))
    out.pop_back ();

  if (out == "[]")
    {
      cerr << "Package not found: " << package << '\n';
      return 1;
    }

  cout << "Cloning " << package << "...\n";
  string ccmd = "git clone " + url + " &> /dev/null";
  if (system (ccmd.c_str ()) != 0)
    {
      cerr << "git clone failed for " << package << '\n';
      return 1;
    }

  cout << "Building " << package << "...\n";
  string mcmd = "cd " + package + " && makepkg -si --noconfirm";
  if (system (mcmd.c_str ()) != 0)
    {
      cerr << "makepkg failed for " << package << '\n';
      return 1;
    }

  return 0;
}

int
remove_pkg (const string &package)
{
  if (!is_installed (package))
    {
      cout << package << " is not installed; skipping removal.\n";
      return 0;
    }
  string cmd = "sudo pacman -Rsn --noconfirm " + package;
  cout << "Removing " << package << "...\n";
  int rc = system (cmd.c_str ());
  if (rc != 0)
    {
      cerr << "Removal failed for " << package << " (code " << rc << ")\n";
      return 1;
    }
  return 0;
}

int
update_pkg (const string &package)
{
  if (package.empty ())
    {
      // full system upgrade
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
      // update single package via pacman if available in repos;
      // for AUR packages, rebuild using makepkg
      if (is_installed (package))
        {
          cout << "Updating repo package " << package << "...\n";
          int rc = system (("sudo pacman -S --noconfirm " + package).c_str ());
          if (rc == 0)
            return 0;
          // Fall back to AUR rebuild
        }
      cout << "Rebuilding AUR package " << package << "...\n";
      string url = "https://aur.archlinux.org/" + package + ".git";
      string tmpdir = "/tmp/auh_" + package;
      // clean, clone, build, install
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
      system (("rm -rf " + tmpdir).c_str ());
      return 0;
    }
}

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

int
build_from_github (const std::string &package,
                   const std::string &mirror_url_base
                   = "https://github.com/archlinux/aur")
{
  // tmp working dir
  const std::string tmpdir = "./auh_mirror_" + package;
  // ensure clean tmpdir
  std::string rm = "rm -rf " + tmpdir;
  system (rm.c_str ());

  // clone mirror (shallow, no tags) into tmpdir
  std::string clone_cmd = "git clone --single-branch --branch " + package
                          + " --depth=1 " + mirror_url_base + ".git " + tmpdir
                          + " 2>/dev/null";
  if (system (clone_cmd.c_str ()) != 0)
    {
      std::cerr << "Failed to clone mirror for " << package << '\n';
      return 1;
    }

  // run makepkg in tmpdir (as normal user). Use --noconfirm and skip PGP if
  // desired.
  std::string mkcmd
      = "cd " + tmpdir + " && makepkg -si --noconfirm --skippgpcheck";
  int mkrc = system (mkcmd.c_str ());

  // cleanup
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

bool
is_aur_up ()
{
  const std::string url = "https://aur.archlinux.org";

  // Get HTTP status code
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

  // Determine status
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

int
install_packages_parallel (const vector<string> &packages, bool use_aur)
{
  // Install packages in parallel using fork with a limit on concurrent
  // processes
  const size_t max_concurrent = 4; // Limit to 4 concurrent installations
  vector<pid_t> children;
  size_t pkg_idx = 0;
  int failed_count = 0;

  while (pkg_idx < packages.size () || !children.empty ())
    {
      // Start new processes up to the limit
      while (pkg_idx < packages.size () && children.size () < max_concurrent)
        {
          const string &pkg = packages[pkg_idx];

          // Validate package name before processing
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
              // Child process
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
              // Parent process - store child PID
              children.push_back (pid);
              pkg_idx++;
            }
          else
            {
              cerr << "Failed to fork for package: " << pkg << '\n';
              failed_count++;
              pkg_idx++;
            }
        }

      // Wait for at least one child to complete
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

              if (WIFEXITED (status) && WEXITSTATUS (status) != 0)
                {
                  failed_count++;
                }
            }
        }
    }

  if (failed_count > 0)
    {
      cerr << failed_count << " package(s) failed to install.\n";
      return 1;
    }

  return 0;
}

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

  // Parse packages using stringstream for cleaner parsing
  istringstream stream (explicit_pkgs);
  string pkg;
  int synced_count = 0;

  while (getline (stream, pkg))
    {
      // Trim any trailing whitespace
      while (!pkg.empty () && isspace ((unsigned char)pkg.back ()))
        pkg.pop_back ();

      if (pkg.empty ())
        continue;

      // Validate package name
      if (!is_valid_package_name (pkg))
        {
          cerr << "Skipping invalid package name: " << pkg << '\n';
          continue;
        }

      // Check if package exists in AUR
      string pcmd = "curl -s "
                    "\"https://aur.archlinux.org/rpc/?v=5&type=info&arg="
                    + pkg + "\" | jq -r '.results | length'";
      string result = run_capture (pcmd);

      // Trim whitespace
      while (!result.empty () && isspace ((unsigned char)result.back ()))
        result.pop_back ();

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

int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      cout << "Usage: auh <install|installg|remove|update|clean|sync> "
              "[packages...]\n";
      return 1;
    }

  string cmd = argv[1];

  if (cmd == "install")
    {
      if (argc < 3)
        {
          cout << "Usage: auh install <packages...>\n";
          return 1;
        }
      // Check AUR status once before processing packages
      bool aur_available = is_aur_up ();

      // Collect all packages into a vector
      vector<string> packages;
      for (int i = 2; i < argc; ++i)
        {
          packages.push_back (argv[i]);
        }

      // Install packages in parallel
      install_packages_parallel (packages, aur_available);
    }
  else if (cmd == "installg")
    {
      if (argc < 3)
        {
          cout << "Usage: auh installg <packages...>\n";
          return 1;
        }
      // Collect all packages into a vector
      vector<string> packages;
      for (int i = 2; i < argc; ++i)
        {
          packages.push_back (argv[i]);
        }

      // Install packages in parallel from GitHub
      install_packages_parallel (packages, false);
    }
  else if (cmd == "remove")
    {
      if (argc < 3)
        {
          cout << "Usage: auh remove <packages...>\n";
          return 1;
        }
      for (int i = 2; i < argc; ++i)
        remove_pkg (argv[i]);
    }
  else if (cmd == "update")
    {
      if (argc == 2)
        {
          // full system update
          update_pkg ("");
        }
      else
        {
          for (int i = 2; i < argc; ++i)
            update_pkg (argv[i]);
        }
    }
  else if (cmd == "clean")
    {
      clean_cache ();
    }
  else if (cmd == "sync")
    {
      sync_explicit ();
    }
  else
    {
      cout << "Unknown command: " << cmd
           << "\nUsage: auh <install|installg|remove|update|clean|sync> "
              "[packages...]\n";
      return 1;
    }

  return 0;
}
