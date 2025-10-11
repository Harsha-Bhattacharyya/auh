#include <array>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

static string run_capture(const string &cmd) {
  array<char, 4096> buf;
  string out;
  unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe)
    return {};
  while (fgets(buf.data(), buf.size(), pipe.get()))
    out += buf.data();
  return out;
}

static bool is_installed(const string &package) {
  string cmd = "pacman -Q " + package + " > /dev/null 2>&1";
  return system(cmd.c_str()) == 0;
}

int install_pkg(const string &package, const string &url) {
  if (is_installed(package)) {
    cout << package << " is already installed; skipping.\n";
    return 0;
  }

  string pcmd =
      "curl -s \"https://aur.archlinux.org/rpc/?v=5&type=info&arg=" + package +
      "\" | jq -c .results";
  string out = run_capture(pcmd);
  while (!out.empty() && isspace((unsigned char)out.back()))
    out.pop_back();

  if (out == "[]") {
    cerr << "Package not found: " << package << '\n';
    return 1;
  }

  cout << "Cloning " << package << "...\n";
  string ccmd = "git clone " + url + " &> /dev/null";
  if (system(ccmd.c_str()) != 0) {
    cerr << "git clone failed for " << package << '\n';
    return 1;
  }

  cout << "Building " << package << "...\n";
  string mcmd = "cd " + package + " && makepkg -si --noconfirm";
  if (system(mcmd.c_str()) != 0) {
    cerr << "makepkg failed for " << package << '\n';
    return 1;
  }

  return 0;
}

int remove_pkg(const string &package) {
  if (!is_installed(package)) {
    cout << package << " is not installed; skipping removal.\n";
    return 0;
  }
  string cmd = "sudo pacman -Rsn --noconfirm " + package;
  cout << "Removing " << package << "...\n";
  int rc = system(cmd.c_str());
  if (rc != 0) {
    cerr << "Removal failed for " << package << " (code " << rc << ")\n";
    return 1;
  }
  return 0;
}

int update_pkg(const string &package) {
  if (package.empty()) {
    // full system upgrade
    cout << "Performing full system upgrade...\n";
    int rc = system("sudo pacman -Syu --noconfirm");
    if (rc != 0) {
      cerr << "System update failed (code " << rc << ")\n";
      return 1;
    }
    return 0;
  } else {
    // update single package via pacman if available in repos;
    // for AUR packages, rebuild using makepkg
    if (is_installed(package)) {
      cout << "Updating repo package " << package << "...\n";
      int rc = system(("sudo pacman -S --noconfirm " + package).c_str());
      if (rc == 0)
        return 0;
      // Fall back to AUR rebuild
    }
    cout << "Rebuilding AUR package " << package << "...\n";
    string url = "https://aur.archlinux.org/" + package + ".git";
    string tmpdir = "/tmp/auh_" + package;
    // clean, clone, build, install
    int rc = system(("rm -rf " + tmpdir + " && git clone " + url + " " +
                     tmpdir + " &> /dev/null")
                        .c_str());
    if (rc != 0) {
      cerr << "Failed to clone AUR for " << package << '\n';
      return 1;
    }
    rc = system(("cd " + tmpdir + " && makepkg -si --noconfirm").c_str());
    if (rc != 0) {
      cerr << "Rebuild/install failed for " << package << '\n';
      return 1;
    }
    system(("rm -rf " + tmpdir).c_str());
    return 0;
  }
}

int clean_cache() {
  string cmd = "sudo pacman -Scc";
  int rc = system(cmd.c_str());
  if (rc == 0) {
    cout << "Sucessfully cleaned";
    return 0;
  } else {
    cout << "System cleaning failed\n";
    return 1;
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    cout << "Usage: auh <install|remove|update> [packages...]\n";
    return 1;
  }

  string cmd = argv[1];

  if (cmd == "install") {
    if (argc < 3) {
      cout << "Usage: auh install <packages...>\n";
      return 1;
    }
    for (int i = 2; i < argc; ++i) {
      string pkg = argv[i];
      string url = "https://aur.archlinux.org/" + pkg + ".git";
      install_pkg(pkg, url);
    }
  } else if (cmd == "remove") {
    if (argc < 3) {
      cout << "Usage: auh remove <packages...>\n";
      return 1;
    }
    for (int i = 2; i < argc; ++i)
      remove_pkg(argv[i]);
  } else if (cmd == "update") {
    if (argc == 2) {
      // full system update
      update_pkg("");
    } else {
      for (int i = 2; i < argc; ++i)
        update_pkg(argv[i]);
    }
  } else if (cmd == "clean") {
    clean_cache();
  } else {
    cout << "Unknown command: " << cmd
         << "\nUsage: auh <install|remove|update> [packages...]\n";
    return 1;
  }

  return 0;
}
