#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice*}"

ya() {
    "$ARCADIA/ya" "$@"
}

AMMO_TOOL_DIR="$ARCADIA/alice/begemot/tools/ammo_from_bg_logs"

ya make -r "$AMMO_TOOL_DIR"

TEMP_DIR=`mktemp -d`
TEMP_AMMO_FILE="$TEMP_DIR/ammo_from_bg_logs.txt"

"$AMMO_TOOL_DIR/ammo_from_bg_logs" -n 50000 -o "$TEMP_AMMO_FILE"

ya upload $TEMP_AMMO_FILE
rm "$TEMP_AMMO_FILE"
rmdir "$TEMP_DIR"
