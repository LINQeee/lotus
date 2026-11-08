#!/usr/bin/env bash
set -euo pipefail

prefix=${LOTUS_HOME:-"$HOME/.local/share/lotus"}
sourceDir=${LOTUS_SOURCE:-}
ref=${LOTUS_REF:-main}
installDeps=1
updatePath=1
while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix) prefix=${2:?Missing directory for --prefix}; shift ;;
        --source) sourceDir=${2:?Missing directory for --source}; shift ;;
        --no-deps) installDeps=0 ;;
        --no-path) updatePath=0 ;;
        --help) echo 'Usage: install.sh [--prefix DIR] [--source DIR] [--no-deps] [--no-path]'; exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
    shift
done
mkdir -p "$prefix"
prefix=$(cd "$prefix" && pwd -P)
temp=$(mktemp -d "${TMPDIR:-/tmp}/lotus-install.XXXXXX")
spinner=
stopSpinner() {
    [[ -n $spinner ]] || return 0
    kill "$spinner" 2>/dev/null || :
    wait "$spinner" 2>/dev/null || :
    spinner=
    printf '\r\033[2K'
}
trap 'stopSpinner; rm -rf "$temp"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
log="$prefix/install.log"
: > "$log"
printf '\n\033[1;36m  LOTUS\033[0m  /  setup\n  Install to %s\n\n' "$prefix"

stage() {
    printf '\n\033[1;36m  [%s/7] %s\033[0m\n' "$1" "$2"
}

updateDownload() {
    local file bytes elapsed rate=0
    local files=("$downloadDirectory"/*--"$downloadPackage"--*.incomplete)
    if [[ ${downloadKind:-brew} == pacman ]]; then
        files=("$downloadDirectory"/"$downloadPackage"-*.pkg.tar.*.part
               "$downloadDirectory"/download-*/"$downloadPackage"-*.pkg.tar.*.part)
    fi
    downloadDetail=
    for file in "${files[@]}"; do
        [[ -f $file && $file != *.sig.part ]] || continue
        if [[ ${downloadKind:-brew} == pacman ]]; then
            bytes=$(stat -c %s "$file" 2>/dev/null) || continue
        else
            bytes=$(stat -f %z "$file" 2>/dev/null) || continue
        fi
        elapsed=$((SECONDS - sampledAt))
        if [[ $file == "$lastFile" && $bytes -ge $lastBytes && $elapsed -gt 0 ]]; then
            rate=$(((bytes - lastBytes) / elapsed / 1024))
        fi
        printf -v downloadDetail '%s.%s MiB · %s KiB/s' "$((bytes / 1048576))" "$((bytes % 1048576 * 10 / 1048576))" "$rate"
        lastFile=$file
        lastBytes=$bytes
        return
    done
    lastFile=
}

runStep() {
    local title=$1 started=$SECONDS status=0
    shift
    # Ask for credentials before animation starts so the prompt stays readable
    if [[ $1 == sudo ]]; then sudo -v; fi
    : > "$temp/step.log"
    if [[ ( -t 1 || ${LOTUS_TERMINAL:-0} == 1 ) && ${GITHUB_ACTIONS:-} != true ]]; then
        (
            frames='|/-\'
            i=0
            lastFile= lastBytes=0 sampledAt=-1 downloadDetail=
            while :; do
                detail=$(tail -c 4096 "$temp/step.log" | tr '\r' '\n' | sed -nE 's/^==> (Downloading|Fetching|Pouring|Installing) (.*)/\1 \2/p; s/^.* ([0-9]+([.][0-9]+)?%).*$/Progress \1/p' | tail -1)
                if [[ -n ${downloadDirectory:-} && $SECONDS != "$sampledAt" ]]; then
                    updateDownload
                    sampledAt=$SECONDS
                fi
                if [[ -n $downloadDetail ]]; then detail=$downloadDetail; fi
                printf '\r\033[2K\033[36m  %s %s\033[0m  (%ss)' "${frames:i++%4:1}" "$title" "$((SECONDS - started))"
                width=$((${COLUMNS:-80} - ${#title} - 18))
                if [[ -n $detail && $width -gt 0 ]]; then printf ' · %.*s' "$width" "$detail"; fi
                sleep 0.2
            done
        ) &
        spinner=$!
    else
        printf '\033[36m  → %s…\033[0m\n' "$title"
    fi
    printf '\n--- %s ---\n' "$title" >> "$log"
    if "$@" > "$temp/step.log" 2>&1; then
        status=0
    else
        status=$?
    fi
    stopSpinner
    cat "$temp/step.log" >> "$log"
    if [[ ${GITHUB_ACTIONS:-} == true ]]; then
        printf '::group::%s · details\n' "$title"
        cat "$temp/step.log"
        echo '::endgroup::'
    fi
    if [[ $status != 0 ]]; then
        printf '\033[31m  ✗ %s failed\033[0m\n' "$title" >&2
        tail -40 "$temp/step.log" >&2
        printf '\n  Full log: %s\n' "$log" >&2
        exit "$status"
    fi
    printf '\033[32m  ✓ %s\033[0m  (%ss)\n' "$title" "$((SECONDS - started))"
}

stage 1 'Dependencies'
if [[ $installDeps == 0 ]]; then printf '  Using installed tools (--no-deps)\n'; fi
case $(uname -s) in
    Darwin)
        if [[ $installDeps == 1 ]]; then
            command -v brew >/dev/null || { echo 'Install Homebrew first: https://brew.sh' >&2; exit 1; }
            brewCache=$(brew --cache)
            packages=(cmake ninja llvm@22 git)
            for i in "${!packages[@]}"; do
                package=${packages[i]}
                downloadDirectory="$brewCache/downloads" downloadPackage=$package \
                    runStep "Dependencies [$((i + 1))/${#packages[@]}] · $package" \
                    env HOMEBREW_NO_ASK=1 HOMEBREW_NO_INSTALL_UPGRADE=1 \
                    brew install "homebrew/core/$package"
            done
            export PATH="$(brew --prefix llvm@22)/bin:$PATH"
        fi
        ;;
    Linux)
        if [[ $installDeps == 1 ]]; then
            command -v apt-get >/dev/null || { echo 'Install LLVM 22+, CMake, Ninja, Git and a C/C++ compiler, then use --no-deps' >&2; exit 1; }
            runStep "Update package index" sudo apt-get update
            runStep "Install build tools" sudo apt-get install -y build-essential cmake ninja-build git curl wget lsb-release software-properties-common gnupg ca-certificates
            if ! command -v llvm-config-22 >/dev/null; then
                runStep "Download LLVM installer" curl -fL --progress-bar https://apt.llvm.org/llvm.sh -o "$temp/llvm.sh"
                runStep "Install LLVM 22" sudo bash "$temp/llvm.sh" 22
            fi
            runStep "Install LLVM headers and Clang" sudo apt-get install -y llvm-22-dev clang-22
            export PATH="/usr/lib/llvm-22/bin:$PATH"
        fi
        ;;
    MINGW*|MSYS*)
        [[ ${MSYSTEM:-} == CLANG64 ]] || { echo 'Use MSYS2 CLANG64 or install.ps1' >&2; exit 1; }
        if [[ $installDeps == 1 ]]; then
            pacmanCache=$(pacman-conf CacheDir | head -1)
            packages=(git clang llvm cmake ninja)
            for i in "${!packages[@]}"; do
                package=${packages[i]}
                nativePackage=$package
                if [[ $package != git ]]; then nativePackage=mingw-w64-clang-x86_64-$package; fi
                downloadDirectory=$pacmanCache downloadPackage=$nativePackage downloadKind=pacman \
                    runStep "Dependencies [$((i + 1))/${#packages[@]}] · $package" \
                    pacman -S --needed --noconfirm "$nativePackage"
            done
        fi
        ;;
    *) echo 'Supported platforms: macOS, Linux and Windows (MSYS2 CLANG64)' >&2; exit 1 ;;
esac

stage 2 'Source code'
if [[ -z $sourceDir ]]; then
    sourceDir="$temp/source"
    runStep "Download Lotus" git clone --progress --depth 1 --branch "$ref" https://github.com/LINQeee/lotus.git "$sourceDir"
    runStep "Download core library" git -C "$sourceDir" submodule update --init --progress --depth 1 lotus-core
else
    printf '  Using local source: %s\n' "$sourceDir"
fi
sourceDir=$(cd "$sourceDir" && pwd -P)
[[ -f "$sourceDir/lotus-core/internal/string_runtime.c" ]] || {
    echo 'lotus-core is missing or outdated. Initialize the submodule before installing.' >&2
    exit 1
}
cmake=${CMAKE:-cmake}
cc=${CC:-clang}
cxx=${CXX:-clang++}
stage 3 'Configure'
runStep "Configure release build" "$cmake" -S "$sourceDir" -B "$temp/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DLOTUS_RUNTIME_DIR="$prefix/lib/lotus" \
    -DCMAKE_C_COMPILER="$cc" -DCMAKE_CXX_COMPILER="$cxx" \
    ${LLVM_DIR:+"-DLLVM_DIR=$LLVM_DIR"}
stage 4 'Build'
runStep "Build compiler and runtime" "$cmake" --build "$temp/build" --target compiler
stage 5 'Install'
runStep "Install compiler and runtime" "$cmake" --install "$temp/build"
stage 6 'Verify'
runStep "Verify CLI" "$prefix/bin/lotus" --help

stage 7 'Terminal setup'
if [[ $updatePath == 1 ]]; then
    # A sourced PATH entry works for interactive shells without replacing existing profiles
    printf -v pathLine 'export PATH=%q/bin:$PATH' "$prefix"
    case ${SHELL:-sh} in
        */zsh|zsh) profile="${ZDOTDIR:-$HOME}/.zshrc" ;;
        */bash|bash) profile="$HOME/.bashrc"; [[ $(uname -s) != Darwin ]] || profile="$HOME/.bash_profile" ;;
        */fish|fish)
            profile="$HOME/.config/fish/conf.d/lotus.fish"
            mkdir -p "$(dirname "$profile")"
            printf -v pathLine 'fish_add_path %q/bin' "$prefix"
            ;;
        *) profile="$HOME/.profile" ;;
    esac
    touch "$profile"
    grep -Fqx "$pathLine" "$profile" || printf '\n%s\n' "$pathLine" >> "$profile"
fi
printf '\n\033[1;32m  ✓ Compiler and runtime installed\033[0m\n'
if [[ $updatePath == 1 ]]; then
    printf '  Open a new terminal, then run:\n\n    \033[1mlotus run hello.lt\033[0m\n'
else
    printf '  PATH unchanged · executable: %s/bin/lotus\n' "$prefix"
fi
printf '\n'
