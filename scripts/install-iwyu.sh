#!/usr/bin/env bash
set -euo pipefail

root=$(git rev-parse --show-toplevel)
cd "$root"
version=$(cat .iwyu-version)
if [[ "$version" != 0.26 ]]; then
    echo "Update this installer for IWYU $version before installing." >&2
    exit 1
fi

cmake=${CMAKE:-$(command -v cmake || true)}
cmake=${cmake:-/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake}
llvmConfig=${LLVM_CONFIG:-$(command -v llvm-config-22 || command -v llvm-config || true)}
llvmConfig=${llvmConfig:-/opt/homebrew/opt/llvm/bin/llvm-config}
if [[ "$("$llvmConfig" --version)" != 22.* ]]; then
    echo "IWYU 0.26 requires LLVM 22 development libraries. Set LLVM_CONFIG to llvm-config 22." >&2
    exit 1
fi

mkdir -p .tools/downloads .tools/src
archive="$root/.tools/downloads/iwyu-$version.tar.gz"
if [[ ! -f "$archive" ]]; then
    curl --fail --location --show-error \
        "https://include-what-you-use.org/downloads/include-what-you-use-$version.src.tar.gz" --output "$archive"
fi
printf '%s  %s\n' 5247c0c9a59df9d14e8aa7408ffec4134c6a4aef12f590929111fbfeac930a08 "$archive" | shasum -a 256 --check
tar -xzf "$archive" -C .tools/src
"$cmake" -S .tools/src/include-what-you-use -B .tools/iwyu-build -G 'Unix Makefiles' \
    -DCMAKE_PREFIX_PATH="$("$llvmConfig" --prefix)" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$root/.tools/iwyu" -DIWYU_LINK_CLANG_DYLIB=ON \
    -DIWYU_RESOURCE_RELATIVE_TO=clang -DIWYU_RESOURCE_DIR=../lib/clang/22
"$cmake" --build .tools/iwyu-build --target include-what-you-use --parallel 4
"$cmake" --install .tools/iwyu-build
echo "Installed IWYU $version in .tools/iwyu."
