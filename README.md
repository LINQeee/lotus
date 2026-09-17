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

There is a project formatter configuration included in the repository. When contributing code, **please use the CLion
formatter provided by the project** so that changes remain consistent with the existing codebase.

Before opening a pull request:

1. Make sure the project builds successfully.
2. Check that your changes don't break existing functionality.
3. Format modified code using the project's CLion formatter.
4. Keep changes focused and avoid unrelated refactoring.

There is no strict contribution template at the moment. If you're unsure whether an idea fits Lotus, opening an issue or
discussion first is completely fine.

## License

See [LICENSE](LICENSE) for the license under which Lotus is distributed.

## Status

Lotus is an experimental language and is not intended to replace established languages at this stage.

The compiler, runtime and language design are actively evolving.

If the idea of building a language from the ground up sounds interesting, contributions and experiments are welcome.
