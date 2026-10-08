MWCC
====

[![Build Status]][actions] [![1.0]][progress] [![1.1]][progress] [![1.1p1]][progress] [![1.2.5]][progress]
[![1.2.5n]][progress] [![1.3]][progress]

[Build Status]: https://github.com/rayanht/mwcc/actions/workflows/build.yml/badge.svg
[actions]: https://github.com/rayanht/mwcc/actions/workflows/build.yml
[1.0]: https://decomp.dev/rayanht/mwcc/GC_1_0.svg?mode=shield&measure=code&label=1.0
[1.1]: https://decomp.dev/rayanht/mwcc/GC_1_1.svg?mode=shield&measure=code&label=1.1
[1.1p1]: https://decomp.dev/rayanht/mwcc/GC_1_1p1.svg?mode=shield&measure=code&label=1.1p1
[1.2.5]: https://decomp.dev/rayanht/mwcc/GC_1_2_5.svg?mode=shield&measure=code&label=1.2.5
[1.2.5n]: https://decomp.dev/rayanht/mwcc/GC_1_2_5n.svg?mode=shield&measure=code&label=1.2.5n
[1.3]: https://decomp.dev/rayanht/mwcc/GC_1_3.svg?mode=shield&measure=code&label=1.3
[progress]: https://decomp.dev/rayanht/mwcc

A matching decompilation of `mwcceppc.exe`, the Windows/x86 CodeWarrior compiler for GameCube.

Supported versions:

- `GC_1_0`: GameCube 1.0
- `GC_1_1`: GameCube 1.1
- `GC_1_1p1`: GameCube 1.1p1 (a patched 1.1)
- `GC_1_2_5`: GameCube 1.2.5
- `GC_1_2_5n`: GameCube 1.2.5n (1.2.5 with Ninji's patch)
- `GC_1_3`: GameCube 1.3

Dependencies
============

- Python 3.11+
- [ninja](https://github.com/ninja-build/ninja)
- [unshield](https://github.com/twogood/unshield), to unpack the CodeWarrior Pro 5.3 updater

macOS: `brew install ninja unshield`. Linux: `apt install ninja-build unshield`.

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
- the CodeWarrior Pro 4, 5, 5.3 and 6 Windows/x86 compilers that built them and the Pro 5.3 linker, from the
  Internet Archive (`build/compilers/pro*`)
- the MSL C library and runtime sources of CodeWarrior Pro 5, from the same Internet Archive disc, and the runtime
  sources of its 5.3 updater (`lib/`), with the compiler's own changes to `printf.c`, `time.c`, `time.win32.c`,
  `startup.win32.c`, `ThreadLocalData.c` and `exchand.cpp` applied (`lib/extra`), and the Win32 import library the
  executable links

For 1.1, 1.1p1, 1.2.5 and 1.2.5n, as in [decomp-toolkit](https://github.com/encounter/decomp-toolkit) projects, the
original is split into one object per translation unit, and `ninja` links the executable with the CodeWarrior linker
from the compiled objects of the Matching sources and the split objects of the others, then checks it against the
original's SHA-1. A source is Matching when its object, code and data, links into the original. For the other versions,
each compiled function is compared with the original's; `ninja` fails when a function of a Matching source differs.

`python tools/verify.py` builds and checks every version, and that every function of a source matches.

Diffing
=======

Open the project in [objdiff](https://github.com/encounter/objdiff) after building.

Project structure
=================

- `config/sources.json`: each source's compiler, flags and status (Matching or NonMatching)
- `config/<version>/config.json`: the original executable and its SHA-1; for the versions other than 1.2.5, the
  sources Matching in that version; for 1.3, the compilers that replace Pro 5 and 5.3, and for 1.0, the options that
  replace the sources' (it does not auto-inline)
- `config/<version>/symbols.txt`, `splits.txt` (1.1, 1.1p1, 1.2.5, 1.2.5n): the original's symbols and its
  translation units, in decomp-toolkit's formats
- `config/<version>/functions.json`: each function's address, size and source (none yet for a function not
  decompiled)
- `config/<version>/bindings.json`: the addresses of the data and functions the sources reference
- `config/lmgr326b.def`: the FLEXlm library the compiler imports by ordinal
- `src/`: the compiler (`driver`, `frontend`, `optimizer`, `backend`) and its statically linked C library and
  runtime (`msl`, `runtime`: one source per object of the original, which builds the library's source of it)
- `include/`: headers; `include/libc` is the C library as the compiler's sources see it
- `lib/` (downloaded): the MSL C library and runtime sources, built by `src/msl` and `src/runtime`

The source is formatted with `clang-format` (`uv run clang-format -i FILE`).

License
=======

The project's files are dedicated to the public domain under CC0 (`LICENSE`); it started from
[inspiredrobot/mwcc](https://github.com/inspiredrobot/mwcc), also CC0.
