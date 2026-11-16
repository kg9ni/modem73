#!/usr/bin/env bash
# Apply kg9ni VARA (command/data ports + RETRIES) onto a modem73-win checkout.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PATCH="$ROOT/packaging/windows/modem73-win-vara.patch"
WIN_SRC="${1:-}"

if [[ -z "$WIN_SRC" ]]; then
  echo "Usage: $0 /path/to/modem73-win" >&2
  exit 2
fi
if [[ ! -d "$WIN_SRC" ]]; then
  echo "Not a directory: $WIN_SRC" >&2
  exit 1
fi
if [[ ! -f "$WIN_SRC/Makefile" || ! -f "$WIN_SRC/kiss_tnc.cc" ]]; then
  echo "Does not look like modem73-win: $WIN_SRC" >&2
  exit 1
fi
if [[ -f "$WIN_SRC/vara_port.hh" ]]; then
  echo "vara_port.hh already present in $WIN_SRC — already patched?" >&2
  exit 1
fi

# Prefer git apply (handles new files); fall back to patch -p1
if command -v git >/dev/null && [[ -d "$WIN_SRC/.git" ]]; then
  git -C "$WIN_SRC" apply --index --whitespace=nowarn "$PATCH" || \
    git -C "$WIN_SRC" apply --whitespace=nowarn "$PATCH"
else
  patch -d "$WIN_SRC" -p1 < "$PATCH"
fi

# Always refresh Winsock vara_port.hh from the regenerated copy so RETRIES
# wiring stays in sync with Linux (patch body can lag).
cp -f "$ROOT/packaging/windows/vara_port.hh" "$WIN_SRC/vara_port.hh"

echo "VARA patch applied to $WIN_SRC (vara_port.hh refreshed)"
echo "Next: cd \"$WIN_SRC\" && make -j\$(nproc)   # or push and run GitHub Actions"
