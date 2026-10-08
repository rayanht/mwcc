#!/usr/bin/env python3
"""python tools/implib.py DEF OUTPUT: an import library for the DLL a module-definition file describes, in the format
Microsoft's LIB writes (a descriptor, the null descriptor and null thunk objects, then one short import per export).

Only what the executable imports by ordinal is supported: `EXPORTS name @ordinal NONAME`, imported as data (an __imp_
pointer and no thunk)."""
import re
import struct
import sys
from pathlib import Path


def coff(sections, symbols):
    """A relocatable i386 COFF object: sections (name, flags, data, [(offset, symbol index, type)]); symbols
    (name, value, section number, storage class), each one entry with no auxiliary record."""
    strings = bytearray(4)
    table = bytearray()
    for name, value, number, storage in symbols:
        raw = name.encode()
        if len(raw) <= 8:
            field = raw.ljust(8, b"\0")
        else:
            field = struct.pack("<II", 0, len(strings))
            strings += raw + b"\0"
        table += field + struct.pack("<IhHBB", value, number, 0, storage, 0)
    header_size = 20 + 40 * len(sections)
    body, headers = bytearray(), bytearray()
    for name, flags, data, relocations in sections:
        data_offset = header_size + len(body)
        body += data
        relocation_offset = header_size + len(body) if relocations else 0
        for offset, index, kind in relocations:
            body += struct.pack("<IIH", offset, index, kind)
        headers += struct.pack("<8sIIIIIIHHI", name.encode(), 0, 0, len(data), data_offset, relocation_offset, 0,
                               len(relocations), 0, flags)
    struct.pack_into("<I", strings, 0, len(strings))
    return (struct.pack("<HHIIIHH", 0x14C, len(sections), 0, header_size + len(body), len(symbols), 0, 0) + headers
            + body + table + strings)


def library(dll, exports):
    base = dll.rsplit(".", 1)[0]
    descriptor, null_descriptor, null_thunk = (f"__IMPORT_DESCRIPTOR_{base}", "__NULL_IMPORT_DESCRIPTOR",
                                               f"\x7f{base}_NULL_THUNK_DATA")
    name = dll.encode() + b"\0"
    name += b"\0" * (len(name) & 1)
    members = [
        (coff([(".idata$2", 0xC0300040, bytes(20), [(12, 2, 7), (0, 3, 7), (16, 4, 7)]),
               (".idata$6", 0xC0200040, name, [])],
              [(descriptor, 0, 1, 2), (".idata$2", 0xC0000040, 1, 104), (".idata$6", 0, 2, 3),
               (".idata$4", 0xC0000040, 0, 104), (".idata$5", 0xC0000040, 0, 104), (null_descriptor, 0, 0, 2),
               (null_thunk, 0, 0, 2)]), [descriptor]),
        (coff([(".idata$3", 0xC0300040, bytes(20), [])], [(null_descriptor, 0, 1, 2)]), [null_descriptor]),
        (coff([(".idata$5", 0xC0300040, bytes(4), []), (".idata$4", 0xC0300040, bytes(4), [])],
              [(null_thunk, 0, 1, 2)]), [null_thunk]),
    ]
    for symbol, ordinal in exports:
        strings = f"_{symbol}".encode() + b"\0" + dll.encode() + b"\0"
        # (IMPORT_OBJECT_HEADER: data, imported by ordinal)
        header = struct.pack("<HHHHIIHH", 0, 0xFFFF, 0, 0x14C, 0, len(strings), ordinal, 1 | 0 << 2)
        members.append((header + strings, [f"__imp__{symbol}"]))

    # The archive: the first linker member (big-endian offsets, symbol order), the second (member indices, sorted
    # names), then the members, each named after the DLL.
    symbol_members = [(symbol, i) for i, (_, names) in enumerate(members) for symbol in names]
    first_size = 4 + 4 * len(symbol_members) + sum(len(s) + 1 for s, _ in symbol_members)
    second_size = 4 + 4 * len(members) + 4 + 2 * len(symbol_members) + sum(len(s) + 1 for s, _ in symbol_members)
    offset = 8 + 60 + first_size + (first_size & 1) + 60 + second_size + (second_size & 1)
    offsets = []
    for data, _ in members:
        offsets.append(offset)
        offset += 60 + len(data) + (len(data) & 1)

    def header(name, size):
        return f"{name:<16}{0:<12}{'':<6}{'':<6}{'0':<8}{size:<10}`\n".encode()

    first = struct.pack(f">I{len(symbol_members)}I", len(symbol_members), *(offsets[i] for _, i in symbol_members)) + \
        b"".join(s.encode() + b"\0" for s, _ in symbol_members)
    ordered = sorted(symbol_members)
    second = struct.pack(f"<I{len(members)}II", len(members), *offsets, len(ordered)) + \
        struct.pack(f"<{len(ordered)}H", *(i + 1 for _, i in ordered)) + b"".join(s.encode() + b"\0" for s, _ in ordered)
    out = bytearray(b"!<arch>\n")
    for name, data in [("/", first), ("/", second)] + [(dll + "/", data) for data, _ in members]:
        out += header(name, len(data)) + data + b"\n" * (len(data) & 1)
    return bytes(out)


def main():
    definition, output = sys.argv[1:]
    text = Path(definition).read_text()
    dll = re.search(r"^LIBRARY\s+(\S+)", text, re.M)[1]
    dll = dll if "." in dll else dll + ".dll"
    exports = [(m[1], int(m[2])) for m in re.finditer(r"^\s+(\w+)\s+@(\d+)\s+NONAME\b", text, re.M)]
    Path(output).write_bytes(library(dll, exports))


if __name__ == "__main__":
    main()
