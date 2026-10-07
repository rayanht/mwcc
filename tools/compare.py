#!/usr/bin/env python3
"""python tools/compare.py {extract,compare,report} VERSION

extract: the original's functions and the ranges between them as COFF objects (build/VERSION/target).
compare: each function of a compiled source, its relocations resolved against the original (the executable is not
  relinked), compared byte for byte and its absolute addresses against the original's base relocations; writes
  objdiff.json and fails when a function of a Matching source differs.
report:  objdiff's progress report (build/VERSION/report.json).
"""
import hashlib
import json
import re
import struct
from bisect import bisect_left
import subprocess
import sys
from functools import cache
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from pe import PEFile


def original(version):
    """The original executable, checked against its published SHA-1."""
    config = json.loads(Path(f"config/{version}/config.json").read_text())
    path = Path(config["original"])
    if hashlib.sha1(path.read_bytes()).hexdigest() != config["sha1"]:
        raise SystemExit(f"{path}: SHA-1 is not {config['sha1']}")
    return config, PEFile(path)


@cache
def sources():
    return json.loads(Path("config/sources.json").read_text())


@cache
def version_config(version):
    return json.loads(Path(f"config/{version}/config.json").read_text())


def matching(version, source):
    """Whether SOURCE's exact functions are linked in VERSION: a version can list its Matching sources itself (its
    config's "matching"), else config/sources.json's status decides."""
    if "matching" in version_config(version):
        return source in version_config(version)["matching"]
    return sources().get(source, {}).get("status", "Matching") == "Matching"


def inventory(version):
    """The executable as rows: each function config/VERSION/functions.json maps, and the ranges between them."""
    config, pe = original(version)
    functions = json.loads(Path(f"config/{version}/functions.json").read_text())
    rows = []
    for section in pe.sections:
        code = bool(section.characteristics & 0x20000000)
        if not code and section.name in (".reloc", ".rsrc", ".idata", ".edata"):
            continue
        start, end = section.virtual_address, section.virtual_address + section.virtual_size
        cursor = start
        selected = sorted((f for f in functions if code and start <= int(f["address"], 0) < end),
                          key=lambda f: int(f["address"], 0)) if code else []
        for f in selected:
            address = int(f["address"], 0)
            if address > cursor:
                rows.append(dict(name=f"unknown/{section.name}/{cursor:08x}", address=cursor, size=address - cursor, code=code))
            # (a function without a source is one the decompilation does not have yet)
            unit = str(Path(f["source"]).with_suffix("")) if "source" in f else "unrecovered"
            rows.append(dict(f, address=address, code=True, name=unit + "/" + f["name"],
                             # (C++-mangled names carry no C underscore in COFF)
                             symbol=f["name"] if f["name"].startswith("?") else "_" + f["name"]))
            cursor = address + f["size"]
        if cursor < end:
            rows.append(dict(name=f"unknown/{section.name}/{cursor:08x}", address=cursor, size=end - cursor, code=code))
    for row in rows:
        section = pe.section_for_address(row["address"])
        row["bss"] = bool(section.characteristics & 0x80) and section.file_size == 0
        row["target"] = f"build/{version}/target/{row['name']}.obj"
        if "source" in row:
            row["base"] = f"build/{version}/base/{row['name']}.obj"
    return config, pe, rows


def write_coff(path, data, name=None, code=True, bss_size=0):
    """One section; unknown ranges intentionally have no invented function symbol."""
    strings = bytearray(b"\0" * 4)
    symbols = b""
    if name:
        encoded = name.encode() + b"\0"
        symname = struct.pack("<II", 0, len(strings))
        strings.extend(encoded)
        symbols = symname + struct.pack("<IhHBB", 0, 1, 0x20 if code else 0, 2, 0)
    struct.pack_into("<I", strings, 0, len(strings))
    size = bss_size or len(data)
    section_name = b".text" if code else b".bss" if bss_size else b".data"
    flags = 0x60000020 if code else 0xC0000080 if bss_size else 0xC0000040
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, 60 + len(data), bool(name), 0, 0)
    section = struct.pack(
        "<8sIIIIIIHHI",
        section_name,
        0,
        0,
        size,
        0 if bss_size else 60,
        0,
        0,
        0,
        0,
        flags,
    )
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + section + data + symbols + strings)


def read_object(path):
    """Preserve COFF symbol indices, including auxiliary entries, for linking."""
    data = path.read_bytes()
    machine, nsec, _, symoff, nsym, opts, _ = struct.unpack_from("<HHIIIHH", data)
    if machine != 0x14C or opts:
        raise ValueError(f"{path}: expected relocatable i386 COFF")
    strstart = symoff + nsym * 18
    strings = data[strstart:]

    def name(raw):
        if raw[:4] == b"\0" * 4:
            offset = struct.unpack_from("<I", raw, 4)[0]
            return strings[offset : strings.index(b"\0", offset)].decode()
        return raw.rstrip(b"\0").decode()

    symbols = {}
    i = 0
    while i < nsym:
        off = symoff + i * 18
        val, sec, typ, storage, aux = struct.unpack_from("<IhHBB", data, off + 8)
        symbols[i] = dict(
            name=name(data[off : off + 8]),
            value=val,
            section=sec,
            type=typ,
            storage=storage,
        )
        i += 1 + aux
    sections = []
    for i in range(nsec):
        off = 20 + i * 40
        rawsize, rawoff, reloff = struct.unpack_from("<III", data, off + 16)
        nrel = struct.unpack_from("<H", data, off + 32)[0]
        flags = struct.unpack_from("<I", data, off + 36)[0]
        sections.append(
            dict(
                name=data[off:off + 8].rstrip(b"\0").decode("latin-1"),
                flags=flags,
                code=bool(flags & 0x20),
                # (an uninitialized-data section has no contents, whatever its raw data pointer says: Pro 5 points it
                # at the next section's)
                data=data[rawoff : rawoff + rawsize] if rawoff and not flags & 0x80 else bytes(rawsize),
                relocs=[
                    struct.unpack_from("<IIH", data, reloff + j * 10)
                    for j in range(nrel)
                ],
            )
        )
    return symbols, sections


def c_symbol(name):
    decorated = re.fullmatch(r"[_@]([A-Za-z_]\w*)@\d+", name)
    return "_" + decorated[1] if decorated else name


def function_section(symbols, sections, symbol_name):
    found = [
        s for s in symbols.values() if s["name"] == symbol_name and s["section"] > 0
    ]
    if not found:
        # Win32 stdcall decorates the same C name with its stack-argument size.
        found = [
            s
            for s in symbols.values()
            if s["section"] > 0 and c_symbol(s["name"]) == symbol_name
        ]
    if len(found) != 1:
        raise ValueError("candidate function not emitted uniquely")
    symbol = found[0]
    sec = sections[symbol["section"] - 1]
    begin = symbol["value"]
    # Only code/function boundaries: debug/section labels are not function ends.
    ends = [
        s["value"]
        for s in symbols.values()
        if s["section"] == symbol["section"]
        and s["value"] > begin
        and (s["type"] & 0x20 or s["storage"] == 2)
    ]
    end = min(ends, default=len(sec["data"]))
    return sec, begin, end


def resolve_function(symbols, sections, symbol_name, target_address, addresses, pe, target_size=None):
    sec, begin, end = function_section(symbols, sections, symbol_name)
    # Immediates the original function itself uses: an ambiguous literal
    # (a short string that occurs many times) is resolved to one of these.
    referenced = set()
    if target_size:
        original = pe.read(target_address, target_size)
        referenced = {struct.unpack_from("<I", original, i)[0] for i in range(max(len(original) - 3, 0))}

    def locate(needle, shift=0):
        """Where NEEDLE is in the original: its one occurrence, else the one the function references (at +SHIFT); two
        or more addresses when that is ambiguous."""
        found = pe.find(needle, limit=2)
        if len(found) > 1 and referenced:
            narrowed = [v - shift for v in referenced if pe.contains(v - shift, needle)]
            if len(narrowed) == 1:
                return narrowed
        return found
    section_index = next(i + 1 for i, section in enumerate(sections) if section is sec)
    body = bytearray(sec["data"][begin:end])
    resolutions = []
    for offset, index, kind in sec["relocs"]:
        if not begin <= offset < end:
            continue
        local = offset - begin
        if local + 4 > len(body):
            raise ValueError("relocation crosses function boundary")
        dest = symbols[index]
        addend = struct.unpack_from("<I", body, local)[0]
        # (a binding names a static of this object only when the bound address holds its contents: literal numbers
        # are per translation unit)
        own = dest["storage"] == 3 and dest["section"] > 0 and not sections[dest["section"] - 1].get("code")
        if own and dest["name"] in addresses:
            literal = sections[dest["section"] - 1]["data"]
            following = min((s["value"] for s in symbols.values() if s["section"] == dest["section"]
                             and s["value"] > dest["value"]), default=len(literal))
            text = literal[dest["value"]:following].split(b"\0", 1)[0]
            own = bool(text) and all(32 <= c < 127 for c in text) and not pe.contains(addresses[dest["name"]], text)
        # (a string can occur more than once: of this object's, the copy the original's own instruction refers to,
        # when the string is there)
        referred = None
        if (dest["storage"] == 3 and dest["section"] > 0 and not sections[dest["section"] - 1].get("code") and kind == 6
                and target_size and local + 4 <= len(original)):
            text = sections[dest["section"] - 1]["data"][dest["value"]:].split(b"\0", 1)[0]
            there = (struct.unpack_from("<I", original, local)[0] - addend) & 0xFFFFFFFF
            if text and all(c in (9, 10, 13) or 32 <= c < 127 for c in text) and pe.contains(there, text + b"\0"):
                referred = there
        if referred is not None:
            address = referred
        elif dest["name"] in addresses and not own:
            address = addresses[dest["name"]]
        elif c_symbol(dest["name"]) in addresses and not own:
            address = addresses[c_symbol(dest["name"])]
        elif dest["section"] == section_index and begin <= dest["value"] < end:
            address = target_address + dest["value"] - begin
        elif dest["section"] > 0 and dest["section"] != section_index:
            literal = sections[dest["section"] - 1]
            if literal.get("code"):
                raise ValueError(f"unbound local function: {dest['name']}")
            literal_end = min(
                (other['value'] for other in symbols.values()
                 if other['section'] == dest['section']
                 and other['value'] > dest['value']
                 and not other['name'].startswith('.')),
                default=len(literal['data']),
            )
            payload = bytearray(literal["data"][dest["value"] : literal_end])
            for table_offset, table_index, table_kind in literal['relocs']:
                if table_offset + 4 <= dest['value'] or table_offset >= literal_end:
                    continue
                position = table_offset - dest['value']
                if position < 0 or position + 4 > len(payload) or table_kind not in (6, 7):
                    payload = bytearray()
                    break
                reference = symbols[table_index]
                table_addend = struct.unpack_from('<I', payload, position)[0]
                reference_name = reference['name']
                if reference_name in addresses:
                    resolved = addresses[reference_name] + table_addend
                elif c_symbol(reference_name) in addresses:
                    resolved = addresses[c_symbol(reference_name)] + table_addend
                elif reference['section'] == section_index and begin <= reference['value'] + table_addend < end:
                    resolved = target_address + reference['value'] + table_addend - begin
                elif reference['section'] == -1:
                    resolved = reference['value'] + table_addend
                else:
                    payload = bytearray()
                    break
                if table_kind == 7:
                    resolved -= pe.image_base
                struct.pack_into('<I', payload, position, resolved & 0xffffffff)
            # A literal ends at the next COFF symbol, which in a merged translation unit can lie past the
            # last table entry: zero bytes after the last relocated entry are alignment padding.
            last_entry = max((o + 4 - dest['value'] for o, _, _ in literal['relocs']
                              if dest['value'] <= o < literal_end), default=None)
            if payload and last_entry is not None and len(payload) > last_entry and not any(payload[last_entry:]):
                payload = payload[:last_entry]
            # Verify the entire literal up to the next COFF data symbol, not
            # unrelated literals that happen to share its section. Every entry
            # in this range must resolve and occur together in the original.
            # (uninitialized data has no contents to find it by: only the original's own reference places it)
            matches = locate(bytes(payload)) if payload and not literal.get("flags", 0) & 0x80 else []
            if len(matches) == 1:
                address = matches[0]
            else:
                # COFF section references can point into a merged string pool.
                start = dest["value"] + addend
                tail = literal["data"][start:]
                stop = tail.find(b"\0")
                string = tail[: stop + 1] if stop > 0 else b""
                printable = string and all(
                    c in (9, 10, 13) or 32 <= c < 127 for c in string[:-1]
                )
                hits = (
                    locate(string, addend)
                    if kind == 6
                    and printable
                    and not any(
                        start <= off < start + len(string)
                        for off, _, _ in literal["relocs"]
                    )
                    else []
                )
                if len(hits) != 1 and target_size and local + 4 <= len(original):
                    # Ambiguous or bss literal: take the original's own reference and
                    # accept it only when the literal's contents are found there.
                    raw = struct.unpack_from("<I", original, local)[0]
                    derived = ((raw - addend) if kind == 6 else (raw + target_address + local + 4 - addend)
                               if kind == 20 else None)
                    if derived is not None:
                        derived &= 0xFFFFFFFF
                        try:
                            there = pe.read(derived, len(payload)) if payload else b""
                        except ValueError:
                            there = bytes(len(payload))
                        if there == bytes(payload):
                            hits = [derived + addend]
                if len(hits) != 1:
                    raise ValueError(
                        f"unbound literal: {dest['name']} ({len(matches)} retail blocks, {len(hits)} strings)"
                    )
                address = hits[0] - addend
        elif dest["section"] == -1:
            address = dest["value"]
        else:
            raise ValueError(f"unbound relocation: {dest['name']}")
        addend = struct.unpack_from("<I", body, local)[0]
        if kind == 6:
            value = address + addend
        elif kind == 7:
            value = address - pe.image_base + addend
        elif kind == 20:
            value = address + addend - (target_address + local + 4)
        else:
            raise ValueError(f"unsupported i386 relocation {kind}")
        struct.pack_into("<I", body, local, value & 0xFFFFFFFF)
        resolutions.append(
            dict(
                offset=local,
                symbol=dest["name"],
                index=index,
                address=address,
                kind=kind,
                addend=addend,
            )
        )
    return bytes(body), resolutions




def extract(version):
    _, pe, rows = inventory(version)
    for row in rows:
        data = b"" if row["bss"] else pe.read(row["address"], row["size"])
        write_coff(row["target"], data, row.get("symbol") or (f"__unknown_{row['address']:08x}" if row["code"] else None),
                   row["code"], row["size"] if row["bss"] else 0)
    Path(f"build/{version}/target/ok").touch()


def base_relocations(pe):
    """The addresses of the absolute (HIGHLOW) fixups in the original's base relocation table."""
    coff = struct.unpack_from("<I", pe.data, 0x3C)[0] + 4
    rva, size = struct.unpack_from("<II", pe.data, coff + 20 + 96 + 5 * 8)
    offset = pe.address_to_offset(pe.image_base + rva)
    end, fixups = offset + size, set()
    while offset < end:
        page, block = struct.unpack_from("<II", pe.data, offset)
        for i in range((block - 8) // 2):
            entry = struct.unpack_from("<H", pe.data, offset + 8 + 2 * i)[0]
            if entry >> 12 == 3:
                fixups.add(pe.image_base + page + (entry & 0xFFF))
        offset += block
    return sorted(fixups)


def check(version):
    """(rows, {row name: (body or None, exact)}) for each function with a source: its compiled bytes, relocations
    resolved, and whether they are the original's."""
    _, pe, rows = inventory(version)
    fixups = base_relocations(pe)
    addresses = {name: int(value, 0) for name, value in json.loads(Path(f"config/{version}/bindings.json").read_text()).items()}
    addresses.update({r["symbol"]: r["address"] for r in rows if "symbol" in r})
    # (a static function is its own source's: the same name can be static in several)
    own = {}
    for r in rows:
        if "symbol" in r:
            own.setdefault(r.get("source"), {})[r["symbol"]] = r["address"]
    objects, results = {}, {}
    for row in rows:
        if "source" not in row:
            continue
        if row["source"] not in objects:
            objects[row["source"]] = read_object(Path(f"build/{version}/compiled/{row['source']}.obj"))
        try:
            body, resolutions = resolve_function(*objects[row["source"]], row["symbol"], row["address"],
                                                 {**addresses, **own[row["source"]]}, pe, row["size"])
        except ValueError:
            results[row["name"]] = (None, False)
            continue
        # (the same bytes, and an absolute address exactly where the original's loader fixes one up: a source cannot
        # write an address as a number where the original has a reference, or the reverse)
        results[row["name"]] = (body, body == pe.read(row["address"], row["size"]) and (
            {row["address"] + r["offset"] for r in resolutions if r["kind"] == 6}
            == set(fixups[bisect_left(fixups, row["address"]):bisect_left(fixups, row["address"] + row["size"])])))
    return rows, results


def compare(version):
    rows, results = check(version)
    units, exact, linked, failed, nonmatching, patched = [], 0, 0, [], [], []
    for row in rows:
        meta = {"complete": False, "progress_categories": ["code" if row["code"] else "data"]}
        unit = dict(name=row["name"], target_path=row["target"], metadata=meta)
        units.append(unit)
        if "source" not in row:
            continue
        meta["source_path"] = row["source"]
        body, same = results[row["name"]]
        if body is not None:
            write_coff(row["base"], body, row["symbol"])
            unit["base_path"] = row["base"]
        # (a function that does not link has no base: objdiff shows the target alone)
        exact += same
        if row.get("binary_patch"):
            # (Ninji's hand-written patch in 1.2.5n, not compiler output: never matched)
            patched.append(row["name"])
        elif same and matching(version, row["source"]):
            meta["complete"] = True
            linked += 1
        elif not same:
            (failed if matching(version, row["source"]) else nonmatching).append(row["name"])
    Path("objdiff.json").write_text(json.dumps({
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "min_version": "3.8.0",
        "custom_make": "ninja",
        "build_target": False,
        "build_base": True,
        "watch_patterns": ["src/**/*.c", "src/**/*.cpp", "include/**/*.h", "config/**/*.json"],
        "units": units,
        "progress_categories": [{"id": "code", "name": "Code"}, {"id": "data", "name": "Data"}],
    }, indent=2) + "\n")
    summary = [f"{version}: {exact}/{sum('source' in r for r in rows)} functions exact, {linked} linked"]
    if nonmatching:
        summary.append("  NonMatching: " + " ".join(n.rsplit("/", 1)[1] for n in nonmatching))
    if patched:
        summary.append("  binary patch: " + " ".join(n.rsplit("/", 1)[1] for n in patched))
    summary += ["  not exact: " + name for name in failed]
    print("\n".join(summary))
    if failed:
        raise SystemExit(1)
    Path(f"build/{version}/ok").write_text("\n".join(summary) + "\n")


def report(version):
    subprocess.run(["build/tools/objdiff-cli", "report", "generate", "-o", f"build/{version}/report.json"], check=True)
    report = json.loads(Path(f"build/{version}/report.json").read_text())
    # (an unknown code range carries a symbol only so objdiff keeps its bytes: it is not a function)
    removed = 0
    for unit in report["units"]:
        if unit["name"].startswith("unknown/"):
            removed += unit["measures"].get("total_functions", 0)
            unit.pop("functions", None)
            unit["measures"]["total_functions"] = 0
            unit["measures"]["matched_functions_percent"] = 0.0
    for measures in [report["measures"], *(c["measures"] for c in report.get("categories", []) if c["id"] == "code")]:
        measures["total_functions"] = measures.get("total_functions", 0) - removed
        total = measures["total_functions"]
        measures["matched_functions_percent"] = 100.0 * measures.get("matched_functions", 0) / total if total else 0.0
    Path(f"build/{version}/report.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    command, version = sys.argv[1:3]
    {"extract": extract, "compare": compare, "report": report}[command](version)
