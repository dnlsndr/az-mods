#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Install a built record-streaming library on a deck over SSH, for testing.
#
#   DECK=192.168.2.104 KEY=~/.azmods/ssh/xdjaz_ecdsa ./test/install-deck.sh
#
# It copies the files into the deck's community stage, turns the mod on, approves the
# library, and restarts EP147. Run it only while nothing is playing or recording.
set -eu

: "${DECK:?set DECK to the deck's address}"
: "${KEY:?set KEY to the SSH private key}"
MOD_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SO="$MOD_DIR/out/record-streaming.so"
[ -f "$SO" ] || { echo "build the mod first: $SO is missing" >&2; exit 2; }
[ -f "$MOD_DIR/azmod.json" ] || { echo "azmod.json is missing" >&2; exit 2; }
[ -f "$MOD_DIR/icon.png" ] || { echo "icon.png is missing" >&2; exit 2; }

SSH="ssh -i $KEY -o IdentitiesOnly=yes -o StrictHostKeyChecking=accept-new root@$DECK"
STAGE=/home/root/settings/az-usbload/community
APPROVED=/home/root/settings/azmod-community/approved

$SSH "mkdir -p '$STAGE/record-streaming' '/home/root/settings/azmod-community'"
for f in record-streaming.so azmod.json icon.png; do
    src="$MOD_DIR/$f"
    [ -f "$src" ] || src="$MOD_DIR/out/$f"
    $SSH "cat > '$STAGE/record-streaming/$f'" < "$src"
done

# Turn it on in the stage's enabled list (one id per line, no duplicates).
$SSH "grep -qx record-streaming '$STAGE/enabled' 2>/dev/null || echo record-streaming >> '$STAGE/enabled'"

# Approve this exact build: one line per id, the newest line wins.
HASH=$(shasum -a 256 "$SO" | cut -d ' ' -f1)
$SSH "grep -v '^record-streaming ' '$APPROVED' 2>/dev/null > '$APPROVED.tmp' || true; \
      echo 'record-streaming $HASH' >> '$APPROVED.tmp'; \
      mv '$APPROVED.tmp' '$APPROVED'"

# This restart is intentional; keep the crash loop detector out of it.
$SSH "rm -f /home/root/settings/azmod-community/.starting; touch /home/root/settings/azmod-logs.on; systemctl restart EP147"

echo "waiting for EP147..."
sleep 8
$SSH "cat /tmp/azmod-community/loaded 2>/dev/null; echo; tail -20 /tmp/azmod-community/record-streaming.log 2>/dev/null"
