MWCC
====

[![Build Status]][actions] [![1.2.5]][progress] [![1.2.5n]][progress] [![1.3]][progress]

[Build Status]: https://github.com/rayanht/mwcc/actions/workflows/build.yml/badge.svg
[actions]: https://github.com/rayanht/mwcc/actions/workflows/build.yml
[1.2.5]: https://decomp.dev/rayanht/mwcc/GC_1_2_5.svg?mode=shield&measure=code&label=1.2.5
[1.2.5n]: https://decomp.dev/rayanht/mwcc/GC_1_2_5n.svg?mode=shield&measure=code&label=1.2.5n
[1.3]: https://decomp.dev/rayanht/mwcc/GC_1_3.svg?mode=shield&measure=code&label=1.3
[progress]: https://decomp.dev/rayanht/mwcc

A matching decompilation of `mwcceppc.exe`, the Windows/x86 CodeWarrior compiler for GameCube.

Supported versions:

- `GC_1_2_5`: GameCube 1.2.5
- `GC_1_2_5n`: GameCube 1.2.5n (1.2.5 with Ninji's patch)
- `GC_1_3`: GameCube 1.3

Dependencies
============

- Python 3.11+
- [ninja](https://github.com/ninja-build/ninja)
- [unshield](https://github.com/twogood/unshield), to unpack the CodeWarrior Pro 5.3 updater

macOS: `brew install ninja unshield`. Linux: `apt install ninja-build unshield`.
Windows: `winget install Ninja-build.Ninja`, and unshield through WSL (`wsl sudo apt install unshield`).

[wibo](https://github.com/decompals/wibo) runs the compilers and is downloaded automatically.

Building
========

```sh
python configure.py
ninja
```

For another version, `python configure.py --version GC_1_2_5n` (or `GC_1_3`).

The build downloads what it needs but this repository does not contain:

- the original executables, from the [decomp.dev compiler archive](https://files.decomp.dev), checked against
  their published SHA-1 (`build/compilers/GC`)
- the CodeWarrior Pro 4, 5, 5.3 and 6 Windows/x86 compilers that built them, from the Internet Archive
  (`build/compilers/pro*`)
- the MSL C library and runtime sources of CodeWarrior Pro 5, from the same Internet Archive disc (`lib/`), with
  `printf.c` patched to the revision the compiler was linked with

Each function of the compiled sources is compared with the original's; `ninja` fails when a function of a Matching
source differs. `python tools/verify.py` builds and checks every version.

Diffing
=======

Open the project in [objdiff](https://github.com/encounter/objdiff) after building.

Project structure
=================

- `config/sources.json`: each source's compiler, flags and status (Matching or NonMatching)
- `config/<version>/config.json`: the original executable and its SHA-1; for 1.3, the compiler that replaces Pro 5.3
  and the sources Matching in that version
- `config/<version>/functions.json`: each function's address, size and source (none yet for a function not
  decompiled)
- `config/<version>/bindings.json`: the addresses of the data and functions the sources reference
- `src/`: the compiler (`driver`, `frontend`, `optimizer`, `backend`) and its statically linked C library and
  runtime (`msl`, `runtime`)
- `include/`: headers; `include/libc` is the C library as the compiler's sources see it
- `lib/` (downloaded): the MSL C library and runtime sources of CodeWarrior Pro 5, built by `src/msl` and
  `src/runtime`

The source is formatted with `clang-format` (`uv run clang-format -i FILE`).

License
=======

The project's files are dedicated to the public domain under CC0 (`LICENSE`); it started from
[inspiredrobot/mwcc](https://github.com/inspiredrobot/mwcc), also CC0.
