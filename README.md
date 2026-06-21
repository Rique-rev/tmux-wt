# tmux-wt

A blazingly fast tmux plugin written in modern C++ (C++17) that seamlessly integrates Git Worktrees with tmux sessions. 

`tmux-wt` reads your current Git repository, parses your active worktrees, and displays them in a native tmux interactive menu. Selecting a worktree instantly switches your tmux client to a dedicated session for that branch, creating it in the background if it doesn't already exist.

## Features

- **Native UI:** Uses `tmux display-menu` for a clean, dependency-free interface.
- **High Performance:** Core logic is written in C++17, ensuring zero lag when parsing repositories with numerous worktrees.
- **Session Isolation:** Each worktree gets its own tmux session automatically, keeping your terminal environments and running processes isolated per branch.
- **Context Aware:** Extracts the short SHA-1 and branch names, automatically falling back to the directory name for detached HEAD worktrees.

## Prerequisites

To build and use `tmux-wt`, you need the following installed on your system:

- `tmux` (>= 3.0 recommended for menu support)
- `git`
- `cmake` (>= 3.15)
- A C++17 compatible compiler (`clang++` or `g++`)

## Installation

Since `tmux-wt` relies on a compiled C++ binary for its core logic, it is recommended to clone the repository and build it locally before adding it to your tmux configuration.

**1. Clone the repository**
Choose a location for the plugin (e.g., your generic projects folder or `~/.tmux/plugins/`).

```bash
cd ~/tmux-wt
mkdir build
cd build
cmake ..
cmake --build .
```

## Configure your .tmux.conf

```bash
run-shell "~/tmux-wt/tmux-wt.tmux"
```
## Reload tmux

```bash
tmux source-file ~/.tmux.conf
```

## Configuration
By default, the plugin binds the menu to Prefix + g.

If you wish to change the keybinding (for example, to avoid conflicts with vim-like pane navigation), open the tmux-wt.tmux script in the root of this repository and change the KEY_BINDING variable:

```bash
# Inside tmux-wt.tmux
KEY_BINDING="g" # Change 'g' to your preferred key
```

Note: Remember to reload your ~/.tmux.conf after modifying the script.

## Usage

1. Inside a tmux session, navigate to any directory that is a Git repository.

2. Press your tmux prefix (usually Ctrl+b or Ctrl+a), followed by g.

3. A menu will pop up displaying your current worktrees along with their short SHA-1 hashes.

4. Press the corresponding number (or use the arrow keys and Enter) to jump to that worktree's session.
