# Lotus core

Native runtime library for the [Lotus compiler](https://github.com/LINQeee/lotus).
`lotus.h` defines the native interface; `library.c` implements the runtime functions and string ownership.

Requires CMake 4.2+ and a C11 compiler.

```sh
cmake -S . -B build
cmake --build build --target lotus_core
```

The library is written to `../libs/`, matching the compiler's runtime loader layout.
In the compiler repository, this repository is mounted at `lotus-core/` as a Git submodule.
Run `lotus-test` from the compiler checkout to test the runtime together with the compiler.
