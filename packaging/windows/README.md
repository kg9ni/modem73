# Windows install package (kg9ni VARA fork)

This folder prepares a **Windows 11 zip** of modem73 that includes the VARA
command/data ports + `RETRIES` (same host API as Linux `VARA_PORT.md`).

Upstream ships Windows builds from a **separate repo**, not from this Linux tree:

| Piece | URL |
|-------|-----|
| Upstream Windows port | https://github.com/RFnexus/modem73-win |
| Upstream Windows releases (zip) | https://github.com/RFnexus/modem73-win/releases |
| This Linux fork (VARA source) | https://github.com/kg9ni/modem73 |

There is **no `.msi` / Setup.exe**. Official packages are a portable zip:

`modem73_<version>_win64.zip` → `modem73.exe` + docs. Unzip and run.

## What we already verified here

On Linux we:

1. Took `RFnexus/modem73-win` @ v2.5.0 parity.
2. Ported Winsock `vara_port.hh` + wired CLI/config/`RETRIES`.
3. Cross-compiled with `x86_64-w64-mingw32-g++`.
4. Smoked `wine modem73.exe --help` (shows `--vara-port` / `--vara-retries`).

A preview zip (Hamlib **direct** PTT not linked; `--ptt rigctl` TCP still works) is
kept in the Project store under `artifacts/`, not in git.

## Recommended shipping path (for Ernie)

### A. One-time: own a Windows packaging repo

1. Open https://github.com/RFnexus/modem73-win
2. Click **Fork** → create **`kg9ni/modem73-win`** (same account as this Linux fork).
3. Clone it on any machine with git:

```bash
git clone https://github.com/kg9ni/modem73-win.git
cd modem73-win
```

4. Apply the VARA patch from this repo:

```bash
# From a checkout of kg9ni/modem73:
git -C /path/to/modem73-win apply /path/to/modem73/packaging/windows/modem73-win-vara.patch
```

Or run:

```bash
./packaging/windows/apply-vara-to-modem73-win.sh /path/to/modem73-win
```

5. Commit, push, then either:
   - **GitHub Actions (preferred):** Actions → **Build Windows package** → Run workflow
     (same as upstream `main_win.yml`: llvm-mingw, Hamlib, zip release), or
   - Tag `vX.Y.Z` to publish a Release automatically.

6. Send your friend the Release asset  
   `modem73_<version>_win64.zip`  
   from `https://github.com/kg9ni/modem73-win/releases`.

### B. Friend install (no build)

1. Download the zip from the kg9ni release (or the preview in Project `artifacts/`).
2. Right-click → **Extract All…** to a folder (e.g. `Documents\modem73`).
3. Double-click `modem73.exe` (or run from Command Prompt).
4. For linbpq / BPQ with `DRIVER=VARA`:

```text
modem73.exe --headless --callsign YOURCALL --vara-port 8300
```

Command port **8300**, data **8301**. See `VARA_PORT.md`.

Windows may show SmartScreen for an unsigned exe: **More info** → **Run anyway**.

### C. Build yourself (optional)

**Cross-compile from Linux** (what CI does):

```bash
# Debian/Ubuntu toolchain (or use llvm-mingw like CI)
sudo apt install make g++-mingw-w64-x86-64-posix zip
cd modem73-win
# optional full Hamlib PTT:
#   ./build-hamlib-win.sh
make -j$(nproc) VERSION=2.5.0-vara
mkdir -p dist && cp modem73.exe README.md CONTROL_PORT.md VARA_PORT.md LICENSE dist/
(cd dist && zip ../modem73_2.5.0-vara_win64.zip *)
```

**Native on Windows 11:** install [MSYS2](https://www.msys2.org/), open **MinGW64**:

```bash
pacman -S --needed make mingw-w64-x86_64-gcc
make CXX=g++ CC=gcc WINDRES=windres
```

## Files in this directory

| File | Role |
|------|------|
| `vara_port.hh` | Winsock VARA host port (regenerate with `mk-winsock-vara-port.py`) |
| `modem73-win-vara.patch` | Apply onto a clean `modem73-win` tree |
| `apply-vara-to-modem73-win.sh` | Convenience wrapper for the patch |
| `mk-winsock-vara-port.py` | Rebuild Winsock header from Linux `../../vara_port.hh` |
| `VARA_PORT.md` | Copy of the host-API doc for the zip |

## Windows vs Linux responsibilities

| Step | Where |
|------|--------|
| Maintain VARA logic | `kg9ni/modem73` (Linux) |
| Win32 / PDCurses / Winsock packaging | `kg9ni/modem73-win` (fork) |
| Produce friend-facing zip | GitHub Actions on `modem73-win`, or Linux mingw cross-build |
| Must use a Windows PC? | **No** for build/release. **Yes** only to run/test audio + PTT on real hardware |
| Traditional installer | Not used upstream; zip is enough. Add Inno Setup later only if you want Start Menu shortcuts |
