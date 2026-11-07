```text
 /$$                   /$$                        
| $$                  | $$                        
| $$        /$$$$$$  /$$$$$$   /$$   /$$  /$$$$$$$
| $$       /$$__  $$|_  $$_/  | $$  | $$ /$$_____/
| $$      | $$  \ $$  | $$    | $$  | $$|  $$$$$$ 
| $$      | $$  | $$  | $$ /$$| $$  | $$ \____  $$
| $$$$$$$$|  $$$$$$/  |  $$$$/|  $$$$$$/ /$$$$$$$/
|________/ \______/    \___/   \______/ |_______/ 
                                                  
```

# Lotus [Pre-Alpha]

> A small programming language built around simplicity, native performance and a concise syntax.

Lotus is a programming language designed to keep code expressive and straightforward while compiling to native code
through LLVM.

The project is still under active development. The language is evolving, and some parts of the syntax and standard
library may change.

## Features

* Native code generation through LLVM
* Statically typed variables and expressions
* Simple, concise syntax
* Functions with typed parameters and return values
* Control flow with `if`, `else`, `while`, `break` and `continue`
* Built-in primitive types and strings
* Automatic numeric conversions where applicable
* C-compatible native functions
* A small runtime library

## Example

A small Lotus program:

```lotus
fn sum(a: int, b: int): int {
    return a + b
}

int result = sum(20, 22)

print(result)
```

The syntax is intentionally familiar. If you've worked with languages such as C, Kotlin, Java or similar statically
typed languages, most of Lotus should feel recognizable.

## Getting Started

### Requirements

* CMake
* LLVM
* A C/C++ compiler
* CLion is recommended for development

### Running a program

Place your Lotus code in:

```text
test.lt
```

Then build and run the compiler from the project.

The exact build configuration is provided by the repository, so no additional project setup is required for trying out
the language.

### Lotus Core

Lotus can use functionality provided by its native runtime library, `lotus-core`.

If you need the runtime, build the project located in:

```text
lotus-core/
```

After building, the library is automatically placed in the directory expected by the compiler.

This allows Lotus programs to use functions provided by the native runtime directly.

Example:

```lotus
string name = input()

print(name)
```

The runtime is intentionally kept separate from the compiler itself, making it possible to extend native functionality
without putting everything into the language frontend.

## Project Structure

```text
.
├── src/                # Lotus compiler
├── lotus-core/         # Native runtime library
├── test.lt             # Example / test Lotus program
├── CMakeLists.txt
└── ...
```

The project structure may change as the compiler develops.

## Roadmap

Lotus is still being built, so the roadmap is intentionally open-ended.

Some of the areas currently being worked on or considered:

* [ ] More complete standard library
* [ ] Structs and arrays
* [ ] Improved string handling
* [ ] Expanded type system
* [ ] Compiler test suite
* [ ] More complete documentation
* [ ] Package/module system

The roadmap is not a fixed specification. Ideas, experiments and contributions can influence the direction of the
project.

## Documentation

The README only covers the basics.

Detailed documentation will cover:

* Language syntax
* Types and conversions
* Functions
* Expressions
* Control flow
* Strings
* Runtime functions
* Compiler internals
* Building and extending Lotus

Documentation will be added separately as the language stabilizes.

## Contributing

Contributions are welcome.

Whether it's a compiler fix, a new language feature, runtime functionality, tests, documentation or simply an issue
describing a problem, feel free to contribute.

### Development

The project is developed primarily with **CLion**.

The C code uses **clang-format 22** with the style defined in [`.clang-format`](.clang-format).
The major version is recorded in `.clang-format-version`. Keep the editor and command-line formatter on that version.
Unused and missing direct C includes are checked with **include-what-you-use (IWYU) 0.26**, recorded in `.iwyu-version`.
IWYU uses CMake's compiler flags and understands declarations, types and macros.

```sh
./scripts/install-iwyu.sh        # Install IWYU locally once (requires LLVM 22, CMake, make and Python 3)
./scripts/install-hooks.sh       # Enable Git checks once per clone
./scripts/style.sh --write       # Remove unused includes, add needed direct includes, then format
./scripts/style.sh --check       # Check both without changing files
```

The formatter covers `.c` and `.h` files in `src/` and `lotus-core/`, including new files that are not ignored.
Generated build directories and deprecated experiments are excluded. Lotus `.lt` files are not formatted.
Set `CLANG_FORMAT=/path/to/clang-format` if the executable is not found automatically.
The individual commands are `./scripts/format.sh` and `./scripts/includes.sh`, each with `--write` and `--check`.

IWYU checks sources listed in the root and runtime `CMakeLists.txt` and the project headers they include, for the current
platform and CMake configuration. Register new C source files in CMake so they are built and checked. Inactive platform
branches need a check on that platform. The script configures temporary builds; an existing build directory is not required.
Cleanup is verified in a temporary source tree before applying edits. Needed direct dependencies can be added when an
unused include previously supplied them indirectly. Existing include groups may be adjusted by IWYU.

The installer builds the pinned IWYU release under ignored `.tools/`, without upgrading the system LLVM.
Set `LLVM_CONFIG` or `CMAKE` to their executable paths if needed; this machine's Homebrew LLVM and CLion CMake are
detected automatically. An existing installation can be selected with `IWYU_BINARY`; its matching `iwyu_tool.py` and
`fix_includes.py` must be in the same directory. Python is used by these upstream tools; no Python test suite is required.
`.iwyu.imp` maps private Apple SDK headers to public C headers. For an intentional side-effect include, use
`#include "header.h" // IWYU pragma: keep` with a short explanation of why it must remain.
See the [IWYU documentation](https://include-what-you-use.org/) and
[pragma reference](https://github.com/include-what-you-use/include-what-you-use/blob/master/docs/IWYUPragmas.md).

The pre-commit hook checks formatting and includes in the **staged snapshot**, using the staged configuration and headers.
It does not rewrite files
or stage changes, so partial staging is preserved. After formatting, review and stage the files you want to commit.
Include `.clang-format-version`, `.iwyu-version`, `.iwyu.imp` and the other setup files in the initial setup commit.

The pre-push hook checks the actual outgoing commits, including intermediate commits made since this setup was added.
It uses each commit's source, headers and configuration. Earlier history without the corresponding version file is
skipped, but each pushed branch or commit-tag tip must pass both checks. A later cleanup commit does not repair an
earlier invalid outgoing commit: amend or rebase that commit before pushing. Ref deletion needs no checks.

Local Git hooks can be bypassed or disabled. For enforcement shared by all contributors, run
`./scripts/style.sh --check` in CI with the same tools and make that check required by the hosting service's branch rules.

In CLion, select **Clang-Format** under **Settings → Editor → Code Style → C/C++ → General** and enable the external
formatter with the same version. On this machine its path is `/opt/homebrew/opt/llvm/bin/clang-format`.
Use **Tools → Actions on Save → Reformat code** for formatting on save, or the regular Reformat Code action.
See the [CLion ClangFormat documentation](https://www.jetbrains.com/help/clion/clangformat-as-alternative-formatter.html).

Before opening a pull request:

1. Make sure the project builds successfully.
2. Check that your changes don't break existing functionality.
3. Run `./scripts/style.sh --check` and fix formatting or include issues.
4. Keep changes focused and avoid unrelated refactoring.

There is no strict contribution template at the moment. If you're unsure whether an idea fits Lotus, opening an issue or
discussion first is completely fine.

## License

See [LICENSE](LICENSE) for the license under which Lotus is distributed.

## Status

Lotus is an experimental language and is not intended to replace established languages at this stage.

The compiler, runtime and language design are actively evolving.

If the idea of building a language from the ground up sounds interesting, contributions and experiments are welcome.
