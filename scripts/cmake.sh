#!/usr/bin/env bash

# Shared CMake selection for scripts running from the project root.
cmake=${CMAKE:-cmake}
if [[ -z "${CMAKE:-}" && -f lotus-scripts-config.json ]]; then
    cmake=$(python3 - <<'PY'
import json
import os

with open('lotus-scripts-config.json') as file:
    command = json.load(file).get('cmake', 'cmake')
if not isinstance(command, str) or not command.strip():
    raise SystemExit('cmake in lotus-scripts-config.json must be an executable name or path')
print(os.path.abspath(command) if '/' in command else command)
PY
    )
fi
if ! command -v "$cmake" >/dev/null 2>&1; then
    echo "CMake not found: $cmake. Set CMAKE or cmake in lotus-scripts-config.json." >&2
    exit 1
fi
