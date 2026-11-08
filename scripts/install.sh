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
trap 'rm -rf "$temp"' EXIT

case $(uname -s) in
    Darwin)
        if [[ $installDeps == 1 ]]; then
            command -v brew >/dev/null || { echo 'Install Homebrew first: https://brew.sh' >&2; exit 1; }
            brew install cmake ninja llvm@22 git
            export PATH="$(brew --prefix llvm@22)/bin:$PATH"
        fi
        ;;
    Linux)
        if [[ $installDeps == 1 ]]; then
            command -v apt-get >/dev/null || { echo 'Install LLVM 22+, CMake, Ninja, Git and a C/C++ compiler, then use --no-deps' >&2; exit 1; }
            sudo apt-get update
            sudo apt-get install -y build-essential cmake ninja-build git curl wget lsb-release software-properties-common gnupg ca-certificates
            if ! command -v llvm-config-22 >/dev/null; then
                curl -fsSL https://apt.llvm.org/llvm.sh -o "$temp/llvm.sh"
                sudo bash "$temp/llvm.sh" 22
            fi
            sudo apt-get install -y llvm-22-dev clang-22
            export PATH="/usr/lib/llvm-22/bin:$PATH"
        fi
        ;;
    MINGW*|MSYS*)
        [[ ${MSYSTEM:-} == CLANG64 ]] || { echo 'Use MSYS2 CLANG64 or install.ps1' >&2; exit 1; }
        if [[ $installDeps == 1 ]]; then
            pacman -S --needed --noconfirm git mingw-w64-clang-x86_64-{clang,llvm,cmake,ninja}
        fi
        ;;
    *) echo 'Supported platforms: macOS, Linux and Windows (MSYS2 CLANG64)' >&2; exit 1 ;;
esac

if [[ -z $sourceDir ]]; then
    sourceDir="$temp/source"
    git clone --depth 1 --branch "$ref" https://github.com/LINQeee/lotus.git "$sourceDir"
    git -C "$sourceDir" submodule update --init --depth 1 lotus-core
fi
sourceDir=$(cd "$sourceDir" && pwd -P)
[[ -f "$sourceDir/lotus-core/internal/string_runtime.c" ]] || {
    echo 'lotus-core is missing or outdated. Initialize the submodule before installing.' >&2
    exit 1
}
cmake=${CMAKE:-cmake}
cc=${CC:-clang}
cxx=${CXX:-clang++}
"$cmake" -S "$sourceDir" -B "$temp/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DLOTUS_RUNTIME_DIR="$prefix/lib/lotus" \
    -DCMAKE_C_COMPILER="$cc" -DCMAKE_CXX_COMPILER="$cxx" \
    ${LLVM_DIR:+"-DLLVM_DIR=$LLVM_DIR"}
"$cmake" --build "$temp/build" --target compiler
"$cmake" --install "$temp/build"
"$prefix/bin/lotus" --help

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
printf '\nLotus installed in %s\nOpen a new terminal, then run: lotus run hello.lt\n' "$prefix"
