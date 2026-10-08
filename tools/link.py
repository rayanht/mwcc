#!/usr/bin/env python3
"""python tools/link.py VERSION OUTPUT INPUT...: link the objects and libraries into the executable with the CodeWarrior
Windows/x86 linker the original was linked with, give it the original's time stamp (the linker writes the time it runs)
and check it against the original's SHA-1, writing build/VERSION/ok when it is the original."""
import hashlib
import json
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from pe import PEFile
from split import read_splits


def main():
    version, output, *inputs = sys.argv[1:]
    config = json.loads(Path(f"config/{version}/config.json").read_text())
    subprocess.run(["build/tools/wibo", "build/compilers/pro53/mwld.exe", "-nostdlib", "-subsystem", "console",
                    "-m", "_mainCRTStartup", *inputs, "-o", output, "-map", str(Path(output).with_suffix(".map"))],
                   check=True)
    original = PEFile(Path(config["original"]))
    linked = bytearray(Path(output).read_bytes())
    stamp = struct.unpack_from("<I", original.data, 0x3C)[0] + 8
    linked[stamp:stamp + 4] = original.data[stamp:stamp + 4]
    Path(output).write_bytes(linked)
    if hashlib.sha1(linked).hexdigest() == config["sha1"]:
        units = len(read_splits(f"config/{version}/splits.txt")[1])
        built = sum(path.startswith(f"build/{version}/compiled/") for path in inputs)
        Path(f"build/{version}/ok").write_text(f"{version}: the original, {built} of {units} units linked from source\n")
        return
    # (where the link differs: the units whose ranges hold the first differing bytes)
    ranges = [(start, end, unit) for unit, sections in read_splits(f"config/{version}/splits.txt")[1]
              for _, start, end, _ in sections]
    print(f"{output}: SHA-1 is not {config['sha1']}", file=sys.stderr)
    if len(linked) != len(original.data):
        print(f"  {len(linked)} bytes, not {len(original.data)}", file=sys.stderr)
    shown = set()
    for offset in range(min(len(linked), len(original.data))):
        if linked[offset] == original.data[offset]:
            continue
        section = next((s for s in original.sections if s.file_offset <= offset < s.file_offset + s.file_size), None)
        address = section.virtual_address + offset - section.file_offset if section else None
        unit = next((u for start, end, u in ranges if address is not None and start <= address < end), "-")
        if unit not in shown:
            shown.add(unit)
            print(f"  differs at {f'0x{address:08x}' if address else f'offset 0x{offset:x}'} ({unit})", file=sys.stderr)
            if len(shown) == 10:
                break
    sys.exit(1)


if __name__ == "__main__":
    main()
