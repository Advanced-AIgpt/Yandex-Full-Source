#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/begemot*}"

TEST_BIN="$ARCADIA/alice/begemot/lib/fixlist_index/data/test_coverage/test_code/test_code"

"$TEST_BIN" --data alice5v3.tsv