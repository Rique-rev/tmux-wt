#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BIN_PATH="$CURRENT_DIR/build/tmux-wt"
KEY_BINDING="g"

chmod +x "$BIN_PATH"

tmux bind-key "$KEY_BINDING" run-shell "cd '#{pane_current_path}' && $BIN_PATH | sh"

