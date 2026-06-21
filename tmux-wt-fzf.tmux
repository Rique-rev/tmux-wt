#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
KEY_BINDING="g"

# Check if fzf is available in the system
if ! command -v fzf &> /dev/null; then
    tmux bind-key "$KEY_BINDING" run-shell "tmux display-message 'Error: fzf is not installed. Please install it to use the FZF UI.'"
    exit 0
fi

# Ensure the sub-script has execution permissions
chmod +x "$CURRENT_DIR/fzf-menu.sh"

# Bind the key to open a tmux popup executing the fzf menu script
tmux bind-key "$KEY_BINDING" display-popup -d '#{pane_current_path}' -E "\"$CURRENT_DIR/fzf-menu.sh\" || sleep 10"
