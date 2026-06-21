#include <array>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

struct Worktree {
  std::string path;
  std::string short_sha1;
  std::string branch_name;
};

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

auto main() -> int {

  auto output = execute_command("git worktree list --porcelain");

  if (!output.has_value()) {
    std::cout << "tmux display-message 'Error: It was not possible to read Git "
                 "Worktrees.'\n";
    return 1;
  }

  auto worktrees = parse_worktrees(output.value());

  if (worktrees.empty()) {
    std::cout << "tmux display-message 'No Git Worktree found.'\n";
    return 0;
  }

  auto tmux_cmd = build_tmux_menu_command(worktrees);

  std::cout << tmux_cmd << "\n";

  return 0;
}
