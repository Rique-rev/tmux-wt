#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BIN_PATH="$CURRENT_DIR/build/tmux-wt"

WT_OUTPUT=$( "$BIN_PATH" fzf-ui 2>&1 )
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    tmux display-message "$WT_OUTPUT"
    exit 1
fi

if [ -z "$WT_OUTPUT" ]; then
    tmux display-message "No Git Worktrees found."
    exit 0
fi

SELECTED=$( echo "$WT_OUTPUT" | fzf \
    --delimiter='\t' \
    --with-nth=1 \
    --prompt="  Worktrees ❯ " \
    --layout=reverse-list \
    --border=rounded \
    --info=inline \
    --margin=1 \
    --padding=1 \
    --color="bg+:#313244,bg:#1e1e2e,spinner:#f5e0dc,hl:#f38ba8" \
    --color="fg:#cdd6f4,header:#f38ba8,info:#cba6f7,pointer:#f5e0dc" \
    --color="marker:#b4befe,fg+:#cdd6f4,prompt:#cba6f7,hl+:#f38ba8" \
    --bind "y:execute-silent(tmux set-buffer '{3}')+execute-silent(tmux run-shell -b 'tmux show-buffer | xclip -i -selection clipboard')+execute-silent(tmux display-message 'Worktree {3} copied!')+abort" \
    --bind "enter:accept" )

if [ -n "$SELECTED" ]; then
    ACTION=$(echo "$SELECTED" | awk -F'\t' '{print $2}')
    eval "$ACTION"
fi
