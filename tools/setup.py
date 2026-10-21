#!/usr/bin/env python3
"""Gets a computer ready to build NumPlay and your own NumWorks games: macOS (Apple silicon or Intel), Linux
and Windows. Nothing is installed system-wide and no administrator password is needed: everything goes in
one folder in your home (~/.numplay), and the script tells you the one line that puts it on your PATH.

  python3 tools/setup.py            check what's there and get what's missing
  python3 tools/setup.py --check    only check

What it gets:
- the ARM compiler the calculator needs, xPack's arm-none-eabi-gcc (the version NumPlay's own builds use),
  checked against its published SHA-256 (the idea of LilyTheCuteCatgirl's Windows builder, PR #77);
- nwlink 1.0.0 (NumWorks' tool that turns a program into a .nwa app), with npm, from Node.js;
- the Python packages of the tools and the emulator (Pillow, NumPy, pyelftools, Unicorn, zopfli).
It needs Python 3.9 or newer and Node.js (https://nodejs.org) already installed, and GNU make (on macOS,
`xcode-select --install`; on Windows, see tools/build.py). Rust is only needed for Tetris.
"""
import argparse
import hashlib
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import zipfile

GCC = "15.2.1-1.1"
HOME = os.path.join(os.path.expanduser("~"), ".numplay")
PY_PACKAGES = ["pillow", "numpy", "pyelftools", "unicorn==2.1.4", "zopfli"]
PY_IMPORTS = ["PIL", "numpy", "elftools", "unicorn", "zopfli"]


def say(ok, what, hint=""):
    print(f"  {'ok ' if ok else '-- '} {what}" + (f"\n       {hint}" if hint and not ok else ""))
    return ok


def gcc_asset():
    s, m = sys.platform, platform.machine().lower()
    arm = m in ("arm64", "aarch64")
    if s == "darwin":
        return f"darwin-{'arm64' if arm else 'x64'}", "tar.gz"
    if s.startswith("linux"):
        return f"linux-{'arm64' if arm else 'x64'}", "tar.gz"
    if s in ("win32", "cygwin"):
        return "win32-x64", "zip"
    sys.exit(f"setup: no ARM compiler build for {s} {m}")


def gcc_bin():
    return os.path.join(HOME, f"xpack-arm-none-eabi-gcc-{GCC}", "bin")


def get_gcc():
    plat, ext = gcc_asset()
    name = f"xpack-arm-none-eabi-gcc-{GCC}-{plat}.{ext}"
    url = f"https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v{GCC}/{name}"
    print(f"     downloading {name} (about 150 MB)...")
    os.makedirs(HOME, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=HOME) as d:
        path = os.path.join(d, name)
        urllib.request.urlretrieve(url, path)
        want = urllib.request.urlopen(url + ".sha").read().decode().split()[0].lower()
        h = hashlib.sha256()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(1 << 20), b""):
                h.update(chunk)
        if h.hexdigest() != want:
            sys.exit(f"setup: {name} doesn't match its published SHA-256: not used")
        if ext == "zip":
            with zipfile.ZipFile(path) as z:
                z.extractall(HOME)
        else:
            with tarfile.open(path) as t:
                t.extractall(HOME, filter="tar") if hasattr(tarfile, "data_filter") else t.extractall(HOME)


def node_bin():
    return os.path.join(HOME, "node_modules", ".bin")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true", help="only check, get nothing")
    a = ap.parse_args()
    win = sys.platform in ("win32", "cygwin")
    exe = ".exe" if win else ""
    print(f"NumPlay setup: {sys.platform} {platform.machine()}, tools in {HOME}")

    gcc = os.path.join(gcc_bin(), "arm-none-eabi-gcc" + exe)
    if not os.path.exists(gcc) and not shutil.which("arm-none-eabi-gcc") and not a.check:
        get_gcc()
    have_gcc = say(os.path.exists(gcc) or bool(shutil.which("arm-none-eabi-gcc")),
                   f"ARM compiler (arm-none-eabi-gcc {GCC})", "python3 tools/setup.py downloads it")

    npm = shutil.which("npm")
    say(bool(npm), "Node.js and npm", "install Node.js from https://nodejs.org, then run this again")
    nw = os.path.join(node_bin(), "nwlink" + (".cmd" if win else ""))
    if npm and not os.path.exists(nw) and not shutil.which("nwlink") and not a.check:
        print("     installing nwlink 1.0.0...")
        subprocess.run([npm, "install", "--prefix", HOME, "nwlink@1.0.0"], check=True)
    have_nw = say(os.path.exists(nw) or bool(shutil.which("nwlink")), "nwlink 1.0.0 (makes .nwa apps)",
                  "needs npm: npm install --prefix ~/.numplay nwlink@1.0.0")

    missing = []
    for mod, pkg in zip(PY_IMPORTS, PY_PACKAGES):
        try:
            __import__(mod)
        except ImportError:
            missing.append(pkg)
    if missing and not a.check:
        print(f"     installing {' '.join(missing)} for this Python...")
        r = subprocess.run([sys.executable, "-m", "pip", "install", "--user", *missing])
        if r.returncode:
            print("     (pip refused: install them with your Python's own way, or in a venv)")
        missing = [p for m, p in zip(PY_IMPORTS, PY_PACKAGES) if subprocess.run(
            [sys.executable, "-c", f"import {m}"], capture_output=True).returncode]
    say(not missing, "Python packages (Pillow, NumPy, pyelftools, Unicorn, zopfli)",
        f"{sys.executable} -m pip install {' '.join(missing)}")
    say(bool(shutil.which("make")), "GNU make", "macOS: xcode-select --install; Windows: tools/build.py")
    say(bool(shutil.which("cargo")), "Rust (only for Tetris)", "https://rustup.rs, then: rustup target add "
        "thumbv7em-none-eabihf")

    paths = [p for p in (gcc_bin(), node_bin()) if os.path.isdir(p)]
    os.makedirs(HOME, exist_ok=True)
    if win:
        line = "$env:Path = \"" + ";".join(paths) + ";$env:Path\""
        print(f"\nIn PowerShell, before building:\n  {line}")
    else:
        env = os.path.join(HOME, "env.sh")
        with open(env, "w") as f:
            f.write("# NumPlay's tools (tools/setup.py): source this before building\n")
            f.write(f'export PATH="{":".join(paths)}:$PATH"\n')
        print(f"\nBefore building, in each new terminal:\n  source {env}")
    print("Then:\n  make -C sdk/template            a small game, output/starcatcher.nwa\n"
          "  make -j4 NWLINK=nwlink          NumPlay and every game (build/)\n"
          "See docs/make-a-game.md.")
    return 0 if have_gcc and have_nw else 1


if __name__ == "__main__":
    sys.exit(main())
