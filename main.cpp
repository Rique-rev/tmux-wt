#include <array>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct Worktree {
  std::string path;
  std::string short_sha1;
  std::string branch_name;
};

enum class UiMode { kTmux, kFzf };

/**************************************************************************************************/

std::optional<std::string> execute_command(const std::string& cmd) {

  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);

  if (!pipe) {
    return std::nullopt;
  }

  constexpr std::size_t buffer_size = 128;
  std::array<char, buffer_size> buffer;
  std::string result;

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  int exit_status = pclose(pipe.release());
  if (exit_status != 0) {
    std::cerr << "Failed to execute command: " << cmd << "\n";
    return std::nullopt;
  }

  return result;
}

/**************************************************************************************************/

std::vector<Worktree> parse_worktrees(std::string_view git_output) {

  std::vector<Worktree> worktrees;

  Worktree current_worktree = {};

  while (!git_output.empty()) {
    std::size_t pos = git_output.find("\n");

    std::string_view line = git_output.substr(0, pos);

    if (pos == std::string_view::npos) {
      git_output = "";
    } else {
      git_output.remove_prefix(pos + 1);
    }

    if (line.empty()) {
      if (!current_worktree.path.empty()) {
        worktrees.push_back(current_worktree);
        current_worktree = {};
        continue;
      }
    }

    if (line.rfind("worktree ", 0) == 0) {
      current_worktree.path = std::string(line.substr(9));
      continue;
    }

    if (line.rfind("HEAD ", 0) == 0) {
      if (line.size() >= 12) {
        current_worktree.short_sha1 = std::string(line.substr(5, 7));
      }
      continue;
    }

    if (line.rfind("branch refs/heads/", 0) == 0) {
      current_worktree.branch_name = std::string(line.substr(18));
      continue;
    }
  }

  if (!current_worktree.path.empty()) {
    worktrees.push_back(current_worktree);
  }

  return worktrees;
};

/**************************************************************************************************/

std::string build_tmux_menu_command(const std::vector<Worktree>& worktrees) {
  std::string tmux_cmd = "";
  std::string current_path = std::filesystem::current_path().string();

  tmux_cmd += "tmux display-menu -T \" Git Worktrees \" ";

  int hotkey = 1;

  for (const auto& wt : worktrees) {
    // If "branch" is empty, use the path last name file
    std::string display_name = wt.branch_name;
    if (display_name.empty()) {
      display_name = std::filesystem::path(wt.path).filename().string();
    }

    std::string action = "";

    if (wt.path == current_path) {
      // If the user is already inside this worktree directory:
      // Try to select the target window.
      // If it doesn't exist, rename the current window instead.
      action += "run-shell \\\"tmux select-window -t ':";
      action += display_name;
      action += "' 2>/dev/null || tmux rename-window '";
      action += display_name;
      action += "'\\\"";
    } else {
      // If the user is in a different worktree directory:
      // Try to select the target window.
      // If it doesn't exist, spawn a new window at the target path.
      action += "run-shell \\\"tmux select-window -t ':";
      action += display_name;
      action += "' 2>/dev/null || tmux new-window -n '";
      action += display_name;
      action += "' -c '";
      action += wt.path;
      action += "'\\\"";
    }

    tmux_cmd += "\"";
    tmux_cmd += display_name;
    tmux_cmd += " (";
    tmux_cmd += wt.short_sha1;
    tmux_cmd += ")\" \"";
    tmux_cmd += std::to_string(hotkey++);
    tmux_cmd += "\" \"";
    tmux_cmd += action;
    tmux_cmd += "\" ";
  }

  return tmux_cmd;
};

/**************************************************************************************************/

std::string build_fzf_output(const std::vector<Worktree>& worktrees) {
  std::string output = "";
  std::string current_path = std::filesystem::current_path().string();

  for (const auto& wt : worktrees) {
    std::string display_name = wt.branch_name;
    if (display_name.empty()) {
      display_name = std::filesystem::path(wt.path).filename().string();
    }

    std::string action = "";

    if (wt.path == current_path) {
      action += "tmux select-window -t ':";
      action += display_name;
      action += "' 2>/dev/null || tmux rename-window '";
      action += display_name;
      action += "'";
    } else {
      action += "tmux select-window -t ':";
      action += display_name;
      action += "' 2>/dev/null || tmux new-window -n '";
      action += display_name;
      action += "' -c '";
      action += wt.path;
      action += "'";
    }

    // Format: Display String \t Action Command \t Raw Name \n
    output += display_name + " (" + wt.short_sha1 + ")\t";
    output += action + "\t";
    output += display_name + "\n";
  }

  return output;
}

/**************************************************************************************************/

void print_help() {
  std::cout << "Usage: tmux-wt [OPTIONS]\n\n"
            << "A blazingly fast tmux plugin for managing Git Worktrees.\n\n"
            << "Options:\n"
            << "  tmux-ui    Use the native tmux display-menu UI (Default)\n"
            << "  fzf-ui     Output TSV format for fzf integration\n"
            << "  --help     Display this help message and exit\n";
}

/**************************************************************************************************/

auto main(int argc, char* argv[]) -> int {

  UiMode mode = UiMode::kTmux;

  if (argc > 1) {
    std::string_view arg(argv[1]);

    if (arg == "--help") {
      print_help();
      return 0;
    } else if (arg == "tmux-ui") {
      mode = UiMode::kTmux;
    } else if (arg == "fzf-ui") {
      mode = UiMode::kFzf;
    } else {
      std::cerr << "Error: Invalid argument '" << arg << "'.\n"
                << "Run 'tmux-wt --help' for available options.\n";
      return 1;
    }
  }

  // If there are more than 2 arguments, it's an invalid usage
  if (argc > 2) {
    std::cerr << "Error: Too many arguments provided.\n"
              << "Run 'tmux-wt --help' for available options.\n";
    return 1;
  }

  auto output = execute_command("git worktree list --porcelain");

  if (!output.has_value()) {
    // We only output a tmux display-message if we are in Tmux mode,
    // otherwise we output to stderr to not break fzf pipes.
    if (mode == UiMode::kTmux) {
      std::cout << "tmux display-message 'Error: It was not possible to read "
                   "Git Worktrees.'\n";
    } else {
      std::cerr << "Error: It was not possible to read Git Worktrees.\n";
    }

    return 1;
  }

  auto worktrees = parse_worktrees(output.value());

  if (worktrees.empty()) {
    if (mode == UiMode::kTmux) {
      std::cout << "tmux display-message 'No Git Worktree found.'\n";
    } else {
      std::cerr << "No Git Worktree found.\n";
    }
    return 0;
  }

  if (mode == UiMode::kFzf) {
    std::cout << build_fzf_output(worktrees);
  } else {
    std::cout << build_tmux_menu_command(worktrees);
  }

  return 0;
}
