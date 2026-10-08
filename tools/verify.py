#!/usr/bin/env python3
"""python tools/verify.py: build every version and check it, leaving the build configured for the primary one. A version that
links the executable (config/VERSION/splits.txt) must also have every function of a source match in objdiff's report,
linked or not, or for a version that lists its Matching sources (config.json "matching"), of those; another compares
each function with the original's."""
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))
from configure import PRIMARY, VERSIONS

ok = True
for version in [*(v for v in VERSIONS if v != PRIMARY), PRIMARY]:
    subprocess.run([sys.executable, "configure.py", "--version", version], check=True)
    result = subprocess.run(["ninja"], capture_output=True, text=True)
    if result.returncode == 0 and Path(f"config/{version}/splits.txt").exists():
        result = subprocess.run(["ninja", "progress"], capture_output=True, text=True)
    if result.returncode:
        # (the build's last lines when it fails)
        print(result.stdout[-6000:])
        ok = False
        continue
    print(Path(f"build/{version}/ok").read_text().rstrip())
    if Path(f"config/{version}/splits.txt").exists():
        # (the functions patched by hand in the executable are not their source's code)
        patched = {f["name"] for f in json.loads(Path(f"config/{version}/functions.json").read_text())
                   if f.get("binary_patch")}
        report = json.loads(Path(f"build/{version}/report.json").read_text())
        # (another version's sources are the primary's: only those it links from source are its code)
        listed = json.loads(Path(f"config/{version}/config.json").read_text()).get("matching")
        differing = [f"{unit['name']}/{function['name']}" for unit in report["units"]
                     if unit.get("metadata", {}).get("source_path")
                     and (listed is None or unit["metadata"]["source_path"] in listed)
                     for function in unit.get("functions", [])
                     if function.get("fuzzy_match_percent", 0) < 100 and function["name"].lstrip("_") not in patched]
        measures = report["measures"]
        print(f"  code {measures.get('matched_code_percent', 0):.2f}% matched, "
              f"{measures.get('complete_code_percent', 0):.2f}% linked; "
              f"data {measures.get('matched_data_percent', 0):.2f}% matched, "
              f"{measures.get('complete_data_percent', 0):.2f}% linked")
        for name in differing:
            print(f"  differs: {name}")
        ok &= not differing
sys.exit(0 if ok else 1)
