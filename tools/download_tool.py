#!/usr/bin/env python3
"""python tools/download_tool.py TOOL OUTPUT: fetch a build tool.

  objdiff-cli   objdiff's command line (OUTPUT: the executable)
  wibo          the Win32 loader the compilers run under
  compilers     decomp.dev's compiler archive, with the original executables (OUTPUT: a stamp in the extracted tree)
  pro4, pro5, pro53, pro6
                the CodeWarrior Windows/x86 compilers the sources build with (OUTPUT: mwcc.exe)
  lib           the MSL C library and runtime sources of CodeWarrior Pro 5, which src/msl and src/runtime build
                (OUTPUT: lib/ok)
"""
import io
import json
import platform
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
import zipfile
import zlib
from pathlib import Path

OBJDIFF_TAG = "v3.8.1"
WIBO_TAG = "1.2.0"
COMPILERS_TAG = "20250812"
PRO4_ISO = "https://archive.org/download/cwpro4/CW_PRO_R4.ISO"
PRO5_BIN = "https://archive.org/download/cwpro5/Tools%20-%20Windows%2095%20NT/CW_PRO5.bin"
PRO6_ISO = "https://archive.org/download/codewarrior-6.0/CW_Tools_6.0.iso"
PRO53_UPDATER = ("https://archive.org/download/ftp-metrowerks-updates-archive/ftp_metrowerks_updates.7z/"
                 "Metrowerks/CWWindows5/Pro5.3_Factory_Upd_NoLibs.exe")


def get(url, headers=None):
    for attempt in range(6):
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers=headers or {}), timeout=300) as response:
                return response.read()
        except (urllib.error.URLError, TimeoutError) as error:
            # (the Internet Archive answers 500 now and then: retry server errors, not client ones)
            if attempt == 5 or isinstance(error, urllib.error.HTTPError) and error.code < 500:
                raise
            print(f"{url}: {error}; retrying", file=sys.stderr)
            time.sleep(2 ** attempt)


SERVERS = {}


def fetch(url, headers=None):
    """URL's contents; a file of an Internet Archive item is read from each server that holds the item in turn (its
    download address redirects to one of them, which can be down)."""
    item = re.match(r"https://archive\.org/download/([^/]+)/(.+)", url)
    if not item or ".7z/" in item[2]:
        return get(url, headers)
    if item[1] not in SERVERS:
        metadata = json.loads(get(f"https://archive.org/metadata/{item[1]}"))
        SERVERS[item[1]] = [f"https://{server}{metadata['dir']}/" for server in metadata["workable_servers"]]
    for i, server in enumerate(SERVERS[item[1]]):
        try:
            return get(server + item[2], headers)
        except urllib.error.URLError as error:
            if i == len(SERVERS[item[1]]) - 1:
                raise
            print(f"{server}: {error}; trying the next server", file=sys.stderr)


def objdiff_url():
    system = {"Darwin": "macos", "Linux": "linux", "Windows": "windows"}[platform.system()]
    machine = platform.machine().lower()
    machine = {"amd64": "x86_64", "arm64": "arm64" if system == "macos" else "aarch64"}.get(machine, machine)
    suffix = ".exe" if system == "windows" else ""
    return f"https://github.com/encounter/objdiff/releases/download/{OBJDIFF_TAG}/objdiff-cli-{system}-{machine}{suffix}"


def wibo_url():
    arch = "macos" if platform.system() == "Darwin" else platform.machine().lower()
    return f"https://github.com/decompals/wibo/releases/download/{WIBO_TAG}/wibo-{arch}"


class Disc:
    """Files of an ISO9660 disc image on a server, read by byte ranges (the images are hundreds of megabytes)."""

    BLOCK = 128  # (sectors per request)

    def __init__(self, url, raw):
        self.url, self.blocks = url, {}
        # (a MODE2/2352 image holds 2048 bytes of data 24 bytes into each 2352-byte sector)
        self.sector, self.skip = (2352, 24) if raw else (2048, 0)

    def block(self, index):
        if index not in self.blocks:
            first = index * self.BLOCK * self.sector
            data = fetch(self.url, {"Range": f"bytes={first}-{first + self.BLOCK * self.sector - 1}"})
            self.blocks[index] = b"".join(data[i * self.sector + self.skip:i * self.sector + self.skip + 2048]
                                          for i in range(len(data) // self.sector))
        return self.blocks[index]

    def read(self, lba, size):
        data = b""
        while len(data) < size:
            index, offset = divmod(lba, self.BLOCK)
            chunk = self.block(index)[offset * 2048:]
            if not chunk:
                break
            data += chunk
            lba += self.BLOCK - offset
        return data[:size]

    def file(self, path):
        record = self.read(16, 2048)[156:190]
        for part in path.split("/"):
            directory = self.read(struct.unpack_from("<I", record, 2)[0], struct.unpack_from("<I", record, 10)[0])
            cursor, record = 0, None
            while cursor < len(directory):
                size = directory[cursor]
                if not size:
                    cursor = (cursor // 2048 + 1) * 2048
                    continue
                if directory[cursor + 33:cursor + 33 + directory[cursor + 32]] == part.encode():
                    record = directory[cursor:cursor + size]
                    break
                cursor += size
            if record is None:
                raise SystemExit(f"{self.url}: no {path}")
        return DiscFile(self, struct.unpack_from("<I", record, 2)[0], struct.unpack_from("<I", record, 10)[0])


class DiscFile(io.RawIOBase):
    """A file on a Disc, seekable: zipfile reads only the directory and the member it extracts."""

    def __init__(self, disc, lba, size):
        self.disc, self.lba, self.size, self.pos = disc, lba, size, 0

    def seekable(self):
        return True

    def tell(self):
        return self.pos

    def seek(self, offset, whence=0):
        self.pos = (offset, self.pos + offset, self.size + offset)[whence]
        return self.pos

    def read(self, size=-1):
        size = min(self.size - self.pos if size < 0 else size, self.size - self.pos)
        if size <= 0:
            return b""
        sector, skip = divmod(self.pos, 2048)
        data = self.disc.read(self.lba + sector, size + skip)[skip:]
        self.pos += len(data)
        return data


def tools_zip_member(url, raw, archive, member):
    return zipfile.ZipFile(Disc(url, raw).file(archive)).read(member)


def pro53():
    """mwcc.exe of the Pro 5.3 updater: an InstallShield executable whose data1.cab/data1.hdr are deflated ZIP
    members at these offsets; unshield unpacks the cabinet."""
    updater = fetch(PRO53_UPDATER)
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        for offset in (100746, 40487531):
            _, _, _, _, _, _, packed, _, length, extra = struct.unpack_from("<5H3I2H", updater, offset + 4)
            name = updater[offset + 30:offset + 30 + length].decode()
            begin = offset + 30 + length + extra
            (root / name).write_bytes(zlib.decompress(updater[begin:begin + packed], -15))
        unshield, path = ["unshield"], str
        if not shutil.which("unshield"):
            # (there is no Windows build of unshield: use the one of WSL, which sees drives under /mnt)
            if sys.platform != "win32" or not shutil.which("wsl"):
                raise SystemExit("unshield is required (brew install unshield / apt install unshield)")
            unshield = ["wsl", "-e", "unshield"]
            if subprocess.run(["wsl", "-e", "which", "unshield"], capture_output=True).returncode:
                raise SystemExit("unshield is required in WSL (wsl sudo apt install unshield)")

            def path(p):
                return subprocess.check_output(["wsl", "-e", "wslpath", "-u", p], text=True).strip()
        subprocess.run([*unshield, "-g", "Win CC++ - FU2", "-j", "-d", path(str(root / "files")), "x",
                        path(str(root / "data1.cab")), "mwcc.exe"], check=True, stdout=subprocess.DEVNULL)
        return (root / "files/Win_CC++_-_FU2/mwcc.exe").read_bytes()


# lib/ from the Pro 5 tools archive: (directory, archive directory, files: all when None)
LIB = [
    ("msl/MSL_Common/Include", "MSL/MSL_C/MSL_Common/Include", None),
    ("msl/MSL_Common/Src", "MSL/MSL_C/MSL_Common/Src", None),
    ("msl/MSL_Cpp/MSL_Common/Include", "MSL/MSL_C++/MSL_Common/Include", None),
    ("msl/MSL_Cpp/MSL_Common/Src", "MSL/MSL_C++/MSL_Common/Src", None),
    ("msl/MSL_Win32/Include", "MSL/MSL_C/MSL_Win32/Include", None),
    ("msl/MSL_Win32/Src", "MSL/MSL_C/MSL_Win32/Src", None),
    ("msl/MSL_X86", "MSL/MSL_C/MSL_X86", None),
    ("msl/win32sdk", "Win32-x86 Support/Headers/Win32 SDK",
     ["BaseTsd.h", "POPPACK.H", "PSHPACK2.H", "PSHPACK4.H", "PSHPACK8.H", "TCHAR.H", "WINBASE.H", "WINDEF.H",
      "WINERROR.H", "WINNT.H", "WINUSER.H", "sys/TYPES.H"]),
    ("msl/extra", "MSL/MSL_C/MSL_Common/Include", ["os_enum.h"]),
    ("msl/extra", "Win32-x86 Support/Headers/Win32 SDK", ["x86_prefix.h"]),
    ("msl/extra", "MSL/MSL_C/MSL_Common/Src", ["printf.c"]),
    ("runtime", "Win32-x86 Support/Libraries/Runtime/(Sources)", None),
]


def lib(output):
    root = output.parent
    with zipfile.ZipFile(Disc(PRO5_BIN, True).file("CODEWA~1.ZIP;1")) as archive:
        names = [n for n in archive.namelist() if not n.endswith("/")]
        for directory, source, files in LIB:
            for name in names:
                relative = name[len(source) + 1:] if name.startswith(source + "/") else None
                if relative is None or files is not None and relative not in files:
                    continue
                target = root / directory / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(archive.read(name))
    # (the compiler was linked with an MSL revision whose printf prints a null %s as "(null)": the string its
    # __pformatter references)
    printf = root / "msl/extra/printf.c"
    data = printf.read_bytes()
    old = b'buff_ptr = "";      /*97010mani@be */'
    if data.count(old) != 1:
        raise SystemExit(f"{printf}: unexpected contents")
    printf.write_bytes(data.replace(old, b'buff_ptr = "(null)";      /*97010mani@be */'))
    output.touch()


def main():
    tool, output = sys.argv[1], Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    if sys.platform == "win32" and tool in ("pro5", "pro53", "pro6"):
        # (the compilers check a FlexLM license, which wibo stubs elsewhere: a stub library does here)
        shutil.copy(Path(__file__).parent / "lmgr_stub/lmgr326b.dll", output.parent)
    if tool in ("compilers", "pro4", "pro5", "pro53", "pro6", "lib") and output.exists():
        # (fixed archives: a restored cache need not be fetched again)
        output.touch()
        return
    if tool == "lib":
        lib(output)
        return
    if tool == "compilers":
        with zipfile.ZipFile(io.BytesIO(fetch(f"https://files.decomp.dev/compilers_{COMPILERS_TAG}.zip"))) as archive:
            archive.extractall(output.parent)
        output.touch()
        return
    data = {
        "objdiff-cli": lambda: fetch(objdiff_url()),
        "wibo": lambda: fetch(wibo_url()),
        "pro4": lambda: tools_zip_member(PRO4_ISO, False, "CODEWA~1/CODEWA~1.ZIP;1", "Tools/Command Line Tools/mwcc.exe"),
        "pro5": lambda: tools_zip_member(PRO5_BIN, True, "CODEWA~1.ZIP;1", "Tools/Command Line Tools/mwcc.exe"),
        "pro53": pro53,
        "pro6": lambda: tools_zip_member(PRO6_ISO, True, "CODEWA~1.ZIP;1", "Other Metrowerks Tools/Command Line Tools/mwcc.exe"),
    }[tool]()
    output.write_bytes(data)
    output.chmod(0o755)


if __name__ == "__main__":
    main()
