#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--check | --write | --staged | --ref COMMIT]"
    echo "Format C sources in src/ and lotus-core/ with the project's clang-format version."
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
tempDir=$(mktemp -d "${TMPDIR:-/tmp}/lotus-format.XXXXXX")
trap 'rm -rf "$tempDir"' EXIT

if [[ "$mode" == --ref ]]; then
    ref=$(git rev-parse --verify "$2^{commit}")
fi

coreRef=
case "$mode" in
    --staged) entry=$(git ls-files --stage -- lotus-core) ;;
    --ref) entry=$(git ls-tree "$ref" -- lotus-core) ;;
    *) entry= ;;
esac
if [[ "$entry" == 160000\ * ]]; then
    read -r fileMode field coreRef <<< "${entry%%$'\t'*}"
    [[ "$mode" != --staged ]] || coreRef=$field
fi

readFile() {
    if [[ -n "$coreRef" && "$1" == lotus-core/* ]]; then
        git -C lotus-core show "$coreRef:${1#lotus-core/}"
        return
    fi
    case "$mode" in
        --staged) git show ":$1" ;;
        --ref) git show "$ref:$1" ;;
        *) cat "$1" ;;
    esac
}

if ! requiredVersion=$(readFile .clang-format-version); then
    echo "Missing .clang-format-version in the checked snapshot. Include the formatting setup in your commit." >&2
    exit 1
fi
if [[ ! "$requiredVersion" =~ ^[0-9]+$ ]]; then
    echo ".clang-format-version must contain a clang-format major version, for example 22." >&2
    exit 1
fi

if [[ -n "${CLANG_FORMAT:-}" ]]; then
    candidates=("$CLANG_FORMAT")
else
    candidates=("clang-format-$requiredVersion" clang-format
        /opt/homebrew/opt/llvm/bin/clang-format /usr/local/opt/llvm/bin/clang-format
        "/opt/homebrew/opt/llvm@$requiredVersion/bin/clang-format"
        "/usr/local/opt/llvm@$requiredVersion/bin/clang-format")
fi

formatter=""
for candidate in "${candidates[@]}"; do
    if executable=$(command -v "$candidate") && version=$("$executable" --version); then
        if [[ "$version" =~ version[[:space:]]+([0-9]+) ]] && [[ "${BASH_REMATCH[1]}" == "$requiredVersion" ]]; then
            formatter=$executable
            [[ "$formatter" == /* ]] || formatter="$root/$formatter"
            break
        fi
    fi
done
if [[ -z "$formatter" ]]; then
    echo "clang-format $requiredVersion is required. Install it or set CLANG_FORMAT to its executable path." >&2
    exit 1
fi

case "$mode" in
    --staged)
        git ls-files --cached -z -- src lotus-core > "$tempDir/paths"
        ;;
    --ref)
        git ls-tree -r --name-only -z "$ref" -- src lotus-core > "$tempDir/paths"
        ;;
    *)
        git ls-files --cached --others --exclude-standard -z -- src lotus-core > "$tempDir/paths"
        ;;
esac

formatRoot=$root
if [[ -n "$coreRef" ]]; then
    git -C lotus-core ls-tree -r --name-only -z "$coreRef" |
        while IFS= read -r -d '' file; do printf 'lotus-core/%s\0' "$file"; done >> "$tempDir/paths"
elif [[ "$mode" != --staged && "$mode" != --ref && -e lotus-core/.git ]]; then
    git -C lotus-core ls-files --cached --others --exclude-standard -z |
        while IFS= read -r -d '' file; do printf 'lotus-core/%s\0' "$file"; done >> "$tempDir/paths"
fi
if [[ "$mode" == --staged || "$mode" == --ref ]]; then
    formatRoot="$tempDir/tree"
    mkdir -p "$formatRoot"
    readFile .clang-format > "$formatRoot/.clang-format"
fi

files=()
while IFS= read -r -d '' file; do
    case "$file" in
        src/deprecated_or_experiment_files/*|*/cmake-build-*/*|*/CMakeFiles/*) continue ;;
        *.c|*.h) ;;
        *) continue ;;
    esac
    if [[ "$formatRoot" != "$root" ]]; then
        mkdir -p "$formatRoot/$(dirname "$file")"
        readFile "$file" > "$formatRoot/$file"
    elif [[ ! -f "$file" ]]; then
        continue
    fi
    files+=("$file")
done < "$tempDir/paths"

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No C sources to format."
    exit 0
fi

cd "$formatRoot"
if [[ "$mode" == --write ]]; then
    "$formatter" --style="file:$formatRoot/.clang-format" --fallback-style=none -i "${files[@]}"
    echo "Formatted ${#files[@]} C source files."
else
    if ! "$formatter" --style="file:$formatRoot/.clang-format" --fallback-style=none \
        --dry-run --Werror --ferror-limit=1 "${files[@]}"; then
        echo "Formatting check failed. Run ./scripts/format.sh --write, review the diff, then stage the changes." >&2
        if [[ "$mode" == --ref ]]; then
            echo "Checked commit: $ref. Fix any offending outgoing commit before pushing." >&2
        fi
        exit 1
    fi
    echo "Formatting OK (${#files[@]} C source files)."
fi
