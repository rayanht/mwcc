#!/usr/bin/env python3
"""python tools/split.py VERSION: the original executable as one COFF object per unit (build/VERSION/obj), like
decomp-toolkit's `dol split`, from config/VERSION/symbols.txt and config/VERSION/splits.txt (its formats).

Absolute references are the original's base relocations; relative ones are the rel32 calls and jumps out of a function.
A reference's target is the symbol containing it; one in a section no unit holds (.idata) is an external by name. Code is
a section per function, as the compiler writes it. sections.obj, which the link starts with, gives the linker the
original's order of sections and imports.
"""
import hashlib
import json
import re
import struct
import sys
from bisect import bisect_right
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from pe import PEFile

SECTION_FLAGS = {"code": 0x60000020, "data": 0xC0000040, "rodata": 0x40000040, "bss": 0xC0000080}


def read_symbols(path):
    """[(name, section, address, {option: value})] of a symbols.txt."""
    symbols = []
    for number, line in enumerate(Path(path).read_text().splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        match = re.fullmatch(r"(\S+) = ([^:\s]+):0x([0-9A-Fa-f]+);(?: // (.*))?", line)
        if not match:
            raise SystemExit(f"{path}:{number}: cannot parse: {line}")
        options = dict(option.split(":", 1) if ":" in option else (option, True)
                       for option in (match[4] or "").split())
        symbols.append((match[1], match[2], int(match[3], 16), options))
    return symbols


def read_splits(path):
    """({section: {option: value}}, [(unit, [(section, start, end, {option: value})])]) of a splits.txt."""
    sections, units, current = {}, [], None
    for number, line in enumerate(Path(path).read_text().splitlines(), 1):
        if not line.strip():
            continue
        if not line[0].isspace():
            name = line.rstrip()
            if not name.endswith(":"):
                raise SystemExit(f"{path}:{number}: expected a unit or Sections:")
            current = None if name == "Sections:" else []
            if current is not None:
                units.append((name[:-1], current))
            continue
        fields = line.split()
        options = dict(field.split(":", 1) for field in fields[1:])
        if current is None:
            sections[fields[0]] = options
        else:
            current.append((fields[0], int(options.pop("start"), 16), int(options.pop("end"), 16), options))
    return sections, units


def original(version):
    """The original executable, checked against its published SHA-1."""
    config = json.loads(Path(f"config/{version}/config.json").read_text())
    path = Path(config["original"])
    if hashlib.sha1(path.read_bytes()).hexdigest() != config["sha1"]:
        raise SystemExit(f"{path}: SHA-1 is not {config['sha1']}")
    return config, PEFile(path)


def base_relocations(pe):
    """The addresses of the absolute (HIGHLOW) fixups in the original's base relocation table."""
    coff = struct.unpack_from("<I", pe.data, 0x3C)[0] + 4
    rva, size = struct.unpack_from("<II", pe.data, coff + 20 + 96 + 5 * 8)
    offset = pe.address_to_offset(pe.image_base + rva)
    end, fixups = offset + size, []
    while offset < end:
        page, block = struct.unpack_from("<II", pe.data, offset)
        for i in range((block - 8) // 2):
            entry = struct.unpack_from("<H", pe.data, offset + 8 + 2 * i)[0]
            if entry >> 12 == 3:
                fixups.append(pe.image_base + page + (entry & 0xFFF))
        offset += block
    return sorted(fixups)


# Instruction lengths of 32-bit x86 code. One-byte map: opcode -> (has ModRM, immediate), the immediate a size in
# bytes, "z" (operand-sized), "a" (address-sized), "p" (far pointer) or "grp3" (test's immediate in group 3).
ONE_BYTE = {}
for base in range(0x00, 0x40, 8):
    ONE_BYTE.update({base: (1, 0), base + 1: (1, 0), base + 2: (1, 0), base + 3: (1, 0), base + 4: (0, 1),
                     base + 5: (0, "z"), base + 6: (0, 0), base + 7: (0, 0)})
ONE_BYTE.update({opcode: (0, 0) for opcode in range(0x40, 0x62)})
ONE_BYTE.update({0x62: (1, 0), 0x63: (1, 0), 0x68: (0, "z"), 0x69: (1, "z"), 0x6A: (0, 1), 0x6B: (1, 1),
                 0x6C: (0, 0), 0x6D: (0, 0), 0x6E: (0, 0), 0x6F: (0, 0)})
ONE_BYTE.update({opcode: (0, 1) for opcode in range(0x70, 0x80)})
ONE_BYTE.update({0x80: (1, 1), 0x81: (1, "z"), 0x82: (1, 1), 0x83: (1, 1)})
ONE_BYTE.update({opcode: (1, 0) for opcode in range(0x84, 0x90)})
ONE_BYTE.update({opcode: (0, 0) for opcode in range(0x90, 0xA0)})
ONE_BYTE.update({0x9A: (0, "p"), 0xA0: (0, "a"), 0xA1: (0, "a"), 0xA2: (0, "a"), 0xA3: (0, "a"), 0xA8: (0, 1),
                 0xA9: (0, "z")})
ONE_BYTE.update({opcode: (0, 0) for opcode in (*range(0xA4, 0xA8), *range(0xAA, 0xB0))})
ONE_BYTE.update({opcode: (0, 1) for opcode in range(0xB0, 0xB8)})
ONE_BYTE.update({opcode: (0, "z") for opcode in range(0xB8, 0xC0)})
ONE_BYTE.update({0xC0: (1, 1), 0xC1: (1, 1), 0xC2: (0, 2), 0xC3: (0, 0), 0xC4: (1, 0), 0xC5: (1, 0), 0xC6: (1, 1),
                 0xC7: (1, "z"), 0xC8: (0, 3), 0xC9: (0, 0), 0xCA: (0, 2), 0xCB: (0, 0), 0xCC: (0, 0), 0xCD: (0, 1),
                 0xCE: (0, 0), 0xCF: (0, 0), 0xD0: (1, 0), 0xD1: (1, 0), 0xD2: (1, 0), 0xD3: (1, 0), 0xD4: (0, 1),
                 0xD5: (0, 1), 0xD6: (0, 0), 0xD7: (0, 0)})
ONE_BYTE.update({opcode: (1, 0) for opcode in range(0xD8, 0xE0)})
ONE_BYTE.update({opcode: (0, 1) for opcode in range(0xE0, 0xE8)})
ONE_BYTE.update({0xE8: (0, "z"), 0xE9: (0, "z"), 0xEA: (0, "p"), 0xEB: (0, 1), 0xF1: (0, 0), 0xF4: (0, 0),
                 0xF5: (0, 0), 0xF6: (1, "grp3"), 0xF7: (1, "grp3"), 0xFE: (1, 0), 0xFF: (1, 0)})
ONE_BYTE.update({opcode: (0, 0) for opcode in (*range(0xEC, 0xF0), *range(0xF8, 0xFE))})
TWO_BYTE = {0x00: (1, 0), 0x01: (1, 0), 0x02: (1, 0), 0x03: (1, 0), 0x06: (0, 0), 0x08: (0, 0), 0x09: (0, 0),
            0x0B: (0, 0), 0x0D: (1, 0), 0x0E: (0, 0), 0x0F: (1, 1), 0x1F: (1, 0), 0x31: (0, 0), 0x77: (0, 0),
            0xA0: (0, 0), 0xA1: (0, 0), 0xA2: (0, 0), 0xA3: (1, 0), 0xA4: (1, 1), 0xA5: (1, 0), 0xA8: (0, 0),
            0xA9: (0, 0), 0xAB: (1, 0), 0xAC: (1, 1), 0xAD: (1, 0), 0xAF: (1, 0), 0xB0: (1, 0), 0xB1: (1, 0),
            0xB3: (1, 0), 0xB6: (1, 0), 0xB7: (1, 0), 0xBA: (1, 1), 0xBB: (1, 0), 0xBC: (1, 0), 0xBD: (1, 0),
            0xBE: (1, 0), 0xBF: (1, 0), 0xC0: (1, 0), 0xC1: (1, 0), 0xC7: (1, 0)}
TWO_BYTE.update({opcode: (1, 0) for opcode in (*range(0x10, 0x18), *range(0x20, 0x24), *range(0x28, 0x30),
                                               *range(0x40, 0x70), *range(0x74, 0x77), 0x7E, 0x7F,
                                               *range(0x90, 0xA0), *range(0xD0, 0x100))})
TWO_BYTE.update({opcode: (1, 1) for opcode in range(0x70, 0x74)})
TWO_BYTE.update({opcode: (0, "z") for opcode in range(0x80, 0x90)})
TWO_BYTE.update({opcode: (0, 0) for opcode in range(0xC8, 0xD0)})
PREFIXES = {0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0x66, 0x67, 0xF0, 0xF2, 0xF3}


def branches(code, base):
    """(offset of the displacement, target) of each call, jmp and jcc with a 32-bit displacement in CODE at BASE."""
    out, offset = [], 0
    while offset < len(code):
        start, operand16, address16 = offset, False, False
        while code[offset] in PREFIXES:
            operand16 |= code[offset] == 0x66
            address16 |= code[offset] == 0x67
            offset += 1
        opcode = code[offset]
        offset += 1
        if opcode == 0x0F:
            opcode = 0x0F00 | code[offset]
            offset += 1
            modrm, immediate = TWO_BYTE[opcode & 0xFF]
        else:
            modrm, immediate = ONE_BYTE[opcode]
        if modrm:
            byte = code[offset]
            offset += 1
            mod, rm = byte >> 6, byte & 7
            if immediate == "grp3":
                immediate = ((1 if opcode == 0xF6 else "z") if (byte >> 3) & 7 in (0, 1) else 0)
            if address16:
                offset += {0: 2 if rm == 6 else 0, 1: 1, 2: 2, 3: 0}[mod]
            else:
                if mod != 3 and rm == 4:
                    offset += 1 + (4 if mod == 0 and code[offset] & 7 == 5 else 0)
                offset += {0: 4 if rm == 5 else 0, 1: 1, 2: 4, 3: 0}[mod]
        field = offset
        relative = opcode in (0xE8, 0xE9) or 0x0F80 <= opcode <= 0x0F8F
        if immediate == "z":
            offset += 2 if operand16 and not relative else 4
        elif immediate == "a":
            offset += 2 if address16 else 4
        else:
            offset += 6 if immediate == "p" else immediate
        if relative:
            displacement = int.from_bytes(code[field:field + 4], "little", signed=True)
            out.append((field - start + start, base + offset + displacement))
    return out


class Coff:
    """A relocatable i386 COFF object being written."""

    def __init__(self):
        self.sections, self.symbols, self.strings, self.index, self.section_symbols = [], [], bytearray(4), {}, {}

    def section(self, name, flags, data, size, relocations):
        number = len(self.sections) + 1
        self.sections.append((name, flags, data, size, relocations))
        self.section_symbols[number] = self.add_symbol(name, 0, number, 0, 3,
                                                       struct.pack("<IHHIHBB2x", size, len(relocations), 0, 0, 0, 0, 0))
        return number

    def add_symbol(self, name, value, section, kind, storage, aux=b""):
        encoded = name.encode("latin-1")
        if len(encoded) <= 8:
            raw = encoded.ljust(8, b"\0")
        else:
            raw = struct.pack("<II", 0, len(self.strings))
            self.strings += encoded + b"\0"
        index = sum(1 + len(entry) // 18 - 1 for entry in self.symbols)
        self.symbols.append(raw + struct.pack("<IhHBB", value, section, kind, storage, len(aux) // 18) + aux)
        return index

    def external(self, name):
        if name not in self.index:
            self.index[name] = self.add_symbol(name, 0, 0, 0, 2)
        return self.index[name]

    def write(self, path):
        header_size = 20 + 40 * len(self.sections)
        body, headers = bytearray(), bytearray()
        for name, flags, data, size, relocations in self.sections:
            data_offset = header_size + len(body) if data else 0
            body += data
            relocation_offset = header_size + len(body) if relocations else 0
            for offset, symbol, kind in relocations:
                body += struct.pack("<IIH", offset, self.index_of(symbol), kind)
            headers += struct.pack("<8sIIIIIIHHI", name.encode() if len(name) <= 8 else self.long_name(name), 0, 0,
                                   size, data_offset, relocation_offset, 0, len(relocations), 0, flags)
        symbols = b"".join(self.symbols)
        struct.pack_into("<I", self.strings, 0, len(self.strings))
        count = len(symbols) // 18
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(struct.pack("<HHIIIHH", 0x14C, len(self.sections), 0, header_size + len(body), count, 0, 0)
                         + headers + body + symbols + self.strings)

    def index_of(self, symbol):
        return symbol if isinstance(symbol, int) else self.external(symbol)

    def long_name(self, name):
        offset = len(self.strings)
        self.strings += name.encode() + b"\0"
        return f"/{offset}".encode().ljust(8, b"\0")


def split(version):
    config, pe = original(version)
    symbols = read_symbols(f"config/{version}/symbols.txt")
    sections, units = read_splits(f"config/{version}/splits.txt")
    fixups = base_relocations(pe)

    # (the unit whose range holds each symbol)
    ranges_by_section = {}
    for unit, ranges in units:
        for section, start, end, _ in ranges:
            ranges_by_section.setdefault(section, []).append((start, end, unit))
    for section in ranges_by_section.values():
        section.sort()

    def unit_at(section, address):
        for start, end, unit in ranges_by_section.get(section, []):
            if start <= address < end or start == address == end:
                return unit
        return None

    targets = sorted(((address, int(options.get("size", "0"), 16), name, section, options,
                       unit_at(section, address)) for name, section, address, options in symbols
                      if options.get("type") != "label"), key=lambda entry: entry[0])
    starts = [entry[0] for entry in targets]

    def pe_section(value):
        try:
            return pe.section_for_address(value)
        except ValueError:
            return None

    def resolve(value, unit):
        """The symbol a reference from UNIT to VALUE is written against: one of UNIT's or a global one containing it,
        else (a constant offset out of an object, such as a[-1]) the nearest of UNIT's symbols in that section."""
        hi = bisect_right(starts, value)
        containing = [t for t in targets[max(0, hi - 64):hi] if t[0] == value or t[0] <= value < t[0] + t[1]]
        usable = [t for t in containing if t[5] == unit or t[4].get("scope") != "local"]
        if usable:
            best = max(usable, key=lambda t: (t[0], t[5] == unit))
            return best[2], best[3], best[0], best[4]
        try:
            section = pe.section_for_address(value).name
        except ValueError:
            return None
        own = [t for t in targets[max(0, hi - 512):hi + 512] if t[3] == section and t[5] == unit]
        if not own:
            return None
        best = min(own, key=lambda t: abs(t[0] - value))
        return best[2], best[3], best[0], best[4]

    for unit, ranges in units:
        coff = Coff()
        local = {}
        defined = []
        # (the compiler writes a reference to a static in .bss against the section, as it names none of them)
        bss_start = next((start for section, start, _, _ in ranges if section == ".bss"), None)
        for section, start, end, options in ranges:
            if options.get("common"):
                # (left to the linker's COMMON allocation, as the original's last variables were)
                for name, sec, address, symbol_options in symbols:
                    if sec == section and start <= address < end:
                        local[(name, address)] = coff.add_symbol(name, int(symbol_options["size"], 16), 0, 0, 2)
                continue
            kind = options.get("type", sections[section].get("type"))
            align = int(options.get("align", sections[section].get("align", "4")))
            flags = SECTION_FLAGS[kind] | (align.bit_length() << 20)
            members = [(name, address, options) for name, sec, address, options in symbols
                       if sec == section and (start <= address < end or start == address == end)
                       and unit_at(section, address) == unit]
            # (code is a section per function, as the compiler writes it; the zeros up to the next function are the
            # linker's alignment, any other bytes between functions the first's)
            pieces = [(start, end)]
            functions = sorted((address, int(options["size"], 16)) for _, address, options in members
                               if options.get("type") == "function")
            if kind == "code" and functions and functions[0][0] == start:
                pieces = []
                for i, (address, size) in enumerate(functions):
                    following = functions[i + 1][0] if i + 1 < len(functions) else end
                    stop = address + size
                    if following - stop >= 16 or any(pe.read(stop, following - stop)):
                        stop = following
                    pieces.append((address, stop))
            for piece_start, piece_end in pieces:
                data = b"" if kind == "bss" else pe.read(piece_start, piece_end - piece_start)
                relocations = []
                content = bytearray(data)
                if kind != "bss":
                    lo = bisect_right(fixups, piece_start - 1)
                    for at in fixups[lo:bisect_right(fixups, piece_end - 1)]:
                        value = struct.unpack_from("<I", pe.data, pe.address_to_offset(at))[0]
                        target = resolve(value, unit)
                        holder = [m for m in members if m[1] <= at and piece_start <= m[1] < piece_end]
                        if target is None and pe_section(value) is None and holder:
                            # (a fixup the executable keeps over bytes patched by hand, not an address: written against
                            # the symbol holding it, the addend making up the value)
                            name, address, symbol_options = max(holder, key=lambda m: m[1])
                            target = name, section, address, symbol_options
                        if target is None:
                            raise SystemExit(f"{unit}: no symbol for 0x{value:08x} referenced at 0x{at:08x}")
                        if (target[1] == ".bss" and target[3].get("scope") == "local" and bss_start is not None
                                and unit_at(".bss", target[2]) == unit):
                            target = ".bss", ".bss", bss_start, {"scope": "local"}
                        struct.pack_into("<I", content, at - piece_start, (value - target[2]) & 0xFFFFFFFF)
                        relocations.append((at - piece_start, target, 6))
                if kind == "code":
                    for name, address, options in members:
                        if options.get("type") != "function" or not piece_start <= address < piece_end:
                            continue
                        size = int(options["size"], 16)
                        for field, value in branches(pe.read(address, size), address):
                            if address <= value < address + size:
                                continue
                            target = resolve(value, unit)
                            if target is None:
                                raise SystemExit(f"{unit}: branch at 0x{address + field:08x} to 0x{value:08x} is not "
                                                 "to a symbol")
                            # (a branch into a function, which hand patches make, keeps its offset as the addend)
                            struct.pack_into("<I", content, address + field - piece_start, value - target[2])
                            relocations.append((address + field - piece_start, target, 20))
                number = coff.section(options.get("rename", section), flags, bytes(content) if kind != "bss" else b"",
                                      piece_end - piece_start, relocations)
                if kind == "bss" and section == ".bss":
                    local[(".bss", piece_start)] = coff.section_symbols[number]
                for name, address, symbol_options in members:
                    if not (piece_start <= address < piece_end or piece_start == address == piece_end):
                        continue
                    scope = symbol_options.get("scope", "global")
                    if kind == "bss" and scope == "local":
                        continue
                    kind_bits = 0x20 if symbol_options.get("type") == "function" else 0
                    index = coff.add_symbol(name, address - piece_start, number, kind_bits, 3 if scope == "local" else 2)
                    local[(name, address)] = index
                    defined.append(name)
        # (relocations name their targets: a symbol of this unit by index, any other by name)
        for i, (name, flags, data, size, relocations) in enumerate(coff.sections):
            resolved = []
            for offset, (target, section, address, options), kind in relocations:
                index = local.get((target, address))
                if index is None:
                    if options.get("scope") == "local":
                        raise SystemExit(f"{unit}: reference to {target}, local to another unit")
                    index = coff.external(target)
                resolved.append((offset, index, kind))
            coff.sections[i] = (name, flags, data, size, resolved)
        coff.write(Path(f"build/{version}/obj/{unit}.obj"))
    # (the linker orders the image's sections as they first appear and gives imports their slots in the order objects
    # first name them, dead code included: this object, linked first, lists both in the original's order)
    order = Coff()
    for section, options in sections.items():
        order.section(section, SECTION_FLAGS[options["type"]] | (int(options.get("align", "4")).bit_length() << 20), b"",
                      0, [])
    for _, name in sorted((address, name) for name, section, address, _ in symbols if section == ".idata"):
        order.external(name)
    order.write(Path(f"build/{version}/obj/sections.obj"))
    Path(f"build/{version}/obj/ok").touch()


if __name__ == "__main__":
    split(sys.argv[1])
