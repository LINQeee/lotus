#!/usr/bin/env bash
set -euo pipefail

root=$(git rev-parse --show-toplevel)
cd "$root"
hooksPath=$(git config --get core.hooksPath || true)
if [[ -n "$hooksPath" && "$hooksPath" != .githooks ]]; then
    echo "core.hooksPath already points to '$hooksPath'. Integrate the formatting hooks there first." >&2
    exit 1
fi
if [[ -z "$hooksPath" ]]; then
    for hook in pre-commit pre-push; do
        if [[ -f "$(git rev-parse --git-path "hooks/$hook")" ]]; then
            echo "An existing $hook hook is installed. Integrate it with .githooks before changing hooksPath." >&2
            exit 1
        fi
    done
fi

chmod +x scripts/*.sh .githooks/pre-commit .githooks/pre-push
git config --local core.hooksPath .githooks
echo "Enabled local pre-commit and pre-push formatting, include checks and the full Lotus test suite."
