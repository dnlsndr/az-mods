#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Put the built mod in an AZ-MODS USB's community folder and turn it on.
#
#   ./test/install-usb.sh /Volumes/USB
#
# The AZ-MODS desktop app then shows it under Community Mods > Installed, where its switch
# can also turn it on. The deck asks once to allow the build the first time it runs.
set -eu

USB=${1:?usage: install-usb.sh /path/to/usb}
MOD_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEST="$USB/MODS/community-mods/record-streaming"
CFG="$USB/MODS/azmods.cfg"

[ -d "$USB/MODS" ] || { echo "$USB/MODS does not exist; is this an AZ-MODS USB?" >&2; exit 2; }
[ -f "$MOD_DIR/out/record-streaming.so" ] || { echo "build the mod first: out/record-streaming.so is missing" >&2; exit 2; }

mkdir -p "$DEST"
cp "$MOD_DIR/out/record-streaming.so" "$MOD_DIR/azmod.json" "$MOD_DIR/icon.png" "$DEST/"

# The app's switch writes the same line. Keep an existing value, add the line only once.
if ! grep -q '^[[:space:]]*community-mods\.record-streaming[[:space:]]*=' "$CFG" 2>/dev/null; then
    printf 'community-mods.record-streaming = on\n' >> "$CFG"
fi

echo "installed to $DEST"
ls -l "$DEST"
grep 'community-mods\.record-streaming' "$CFG" || true
echo
echo "Next: in AZ-MODS open Community Mods > Installed, check that Record Streaming is on,"
echo "eject the USB, boot the deck with it, then allow the mod under"
echo "Mod Menu > Community Mods > Installed > Record Streaming."
