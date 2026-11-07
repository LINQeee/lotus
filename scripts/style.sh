#!/usr/bin/env bash
set -euo pipefail

root=$(git rev-parse --show-toplevel)
if [[ "${1:---check}" == --write ]]; then
    "$root/scripts/includes.sh" "$@"
    "$root/scripts/format.sh" --write
else
    "$root/scripts/format.sh" "$@"
    "$root/scripts/includes.sh" "$@"
fi
