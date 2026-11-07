#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--check | --write | --staged | --ref COMMIT]"
    echo "Check or repair C includes with IWYU using the snapshot's CMake configuration."
}

mode=${1:---check}
case "$mode" in
    --check|--write|--staged) [[ $# -le 1 ]] || { usage >&2; exit 2; } ;;
    --ref) [[ $# -eq 2 ]] || { usage >&2; exit 2; } ;;
    --help|-h) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
esac

root=$(git rev-parse --show-toplevel)
cd "$root"
tempDir=$(mktemp -d "${TMPDIR:-/tmp}/lotus-includes.XXXXXX")
tempDir=$(cd "$tempDir" && pwd -P)
trap 'rm -rf "$tempDir"' EXIT
tree="$tempDir/tree"
mkdir -p "$tree"
case "$mode" in
    --staged) git ls-files --cached -z > "$tempDir/paths" ;;
    --ref)
        ref=$(git rev-parse --verify "$2^{commit}")
        git ls-tree -r --name-only -z "$ref" > "$tempDir/paths"
        ;;
    *) git ls-files --cached --others --exclude-standard -z > "$tempDir/paths" ;;
esac
while IFS= read -r -d '' file; do
    case "$file" in
        .tools/*|src/deprecated_or_experiment_files/*|cmake-build-*/*|*/cmake-build-*/*|*/CMakeFiles/*) continue ;;
    esac
    mkdir -p "$tree/$(dirname "$file")"
    case "$mode" in
        --staged) git show ":$file" > "$tree/$file" ;;
        --ref) git show "$ref:$file" > "$tree/$file" ;;
        *) [[ ! -f "$file" ]] || cp "$file" "$tree/$file" ;;
    esac
done < "$tempDir/paths"

if [[ ! -f "$tree/.iwyu-version" || ! -f "$tree/.iwyu.imp" ]]; then
    echo "Missing IWYU configuration in the checked snapshot. Stage .iwyu-version and .iwyu.imp." >&2
    exit 1
fi
requiredVersion=$(cat "$tree/.iwyu-version")
iwyu=${IWYU_BINARY:-"$root/.tools/iwyu/bin/include-what-you-use"}
if [[ ! -x "$iwyu" ]]; then
    iwyu=${IWYU_BINARY:-$(command -v include-what-you-use || true)}
fi
if [[ -z "$iwyu" ]] || ! version=$("$iwyu" --version) || [[ "$version" != "include-what-you-use $requiredVersion "* ]]; then
    echo "IWYU $requiredVersion is required. Run ./scripts/install-iwyu.sh or set IWYU_BINARY." >&2
    exit 1
fi
iwyu=$(command -v "$iwyu")
iwyu=$(cd "$(dirname "$iwyu")" && pwd)/$(basename "$iwyu")
export IWYU_BINARY="$iwyu"
toolDir=$(dirname "$iwyu")
cmake=${CMAKE:-$(command -v cmake || true)}
cmake=${cmake:-/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake}
if [[ "$(uname -s)" == Darwin && -z "${SDKROOT:-}" ]]; then
    export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
fi

builds=()
for project in . lotus-core; do
    [[ -f "$tree/$project/CMakeLists.txt" ]] || continue
    build="$tempDir/build-$project"
    if ! "$cmake" -S "$tree/$project" -B "$build" -G 'Unix Makefiles' \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > "$tempDir/configure.log" 2>&1; then
        cat "$tempDir/configure.log" >&2
        exit 1
    fi
    builds+=("$build")
done
if [[ ${#builds[@]} -eq 0 ]]; then
    echo "No CMake projects found in the checked snapshot." >&2
    exit 1
fi

showReport() {
    awk '!/has correct #includes\/fwd-decls/ && NF' "$tempDir/report"
}

analyze() {
    local build result
    findings=0
    : > "$tempDir/report"
    for build in "${builds[@]}"; do
        result=0
        python3 "$toolDir/iwyu_tool.py" -p "$build" -j 4 -- -w \
            -Xiwyu --no_fwd_decls -Xiwyu --error=2 \
            -Xiwyu "--mapping_file=$tree/.iwyu.imp" \
            -Xiwyu "--check_also=$tree/src/*.h" -Xiwyu "--check_also=$tree/lotus-core/*.h" \
            >> "$tempDir/report" 2>&1 || result=$?
        case "$result" in
            0) ;;
            2) findings=1 ;;
            *) showReport >&2; return 1 ;;
        esac
    done
}

analyze
if [[ "$mode" != --write && "$findings" == 1 ]]; then
    showReport >&2
    echo "Include check failed. Run ./scripts/style.sh --write, review the diff, then stage the changes." >&2
    [[ "$mode" != --ref ]] || echo "Checked commit: $ref" >&2
    exit 1
fi

if [[ "$mode" == --write ]]; then
    # Repair a temporary tree first; never leave half-applied fixes in the working tree.
    for pass in 1 2 3 4; do
        [[ "$findings" == 1 ]] || break
        python3 "$toolDir/fix_includes.py" --nosafe_headers --noreorder --nocomments \
            --only_re="^$tree/(src|lotus-core)/" < "$tempDir/report" > "$tempDir/fixes.log"
        # IWYU sometimes spells newly suggested local headers as absolute paths.
        python3 - "$tree" <<'PY'
import os
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
for directory in (root / 'src', root / 'lotus-core'):
    for path in directory.rglob('*'):
        if path.suffix not in ('.c', '.h'):
            continue
        def relative_include(match):
            return '#include "' + os.path.relpath(match[1], path.parent) + '"'
        text = path.read_text()
        updated = re.sub(r'^#include "(' + re.escape(str(root)) + r'/[^"\n]+)"', relative_include, text, flags=re.M)
        updated = re.sub(r'^#include "(llvm-c/[^"\n]+)"', r'#include <\1>', updated, flags=re.M)
        if text != updated:
            path.write_text(updated)
PY
        analyze
    done
    if [[ "$findings" == 1 ]]; then
        showReport >&2
        echo "IWYU fixes did not converge; working files were left unchanged." >&2
        exit 1
    fi
    while IFS= read -r -d '' file; do
        case "$file" in
            src/deprecated_or_experiment_files/*|*/cmake-build-*/*|*/CMakeFiles/*) continue ;;
            src/*.c|src/*.h|lotus-core/*.c|lotus-core/*.h)
                if [[ -f "$tree/$file" && -f "$root/$file" ]] && ! cmp -s "$tree/$file" "$root/$file"; then
                    cp "$tree/$file" "$root/$file"
                    echo "Updated includes: $file"
                fi
                ;;
        esac
    done < "$tempDir/paths"
fi
echo "Includes OK (CMake sources and their project headers)."
