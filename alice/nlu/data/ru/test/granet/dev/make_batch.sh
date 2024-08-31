#!/usr/bin/env bash

set -xe
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[[ "$ARCADIA" ]] || { ARCADIA="${SCRIPT_DIR%/alice/nlu*}"; }

if [[ -z "$1" || -z "$2" || -z "$3" ]]; then
    echo "Usage: $0 SRC_TSV BATCH INTENT"
    echo "Example:"
    echo "  ../prepare.sh  # checkout data and build granet tool"
    echo "  $0 demo_src_tsv_for_make_batch.tsv my_batch alice.random_number"
    exit 1
fi

SRC_TSV="$1"
BATCH="$2"
INTENT="$3"
shift 3

mkdir -p "$BATCH/base"
mkdir -p "$BATCH/target"

TSV_FILE_NAME="$(basename "$SRC_TSV")"
DEST_TSV="$BATCH/base/$TSV_FILE_NAME"
cp "$SRC_TSV" "$DEST_TSV"

cat >"$BATCH/config.json" <<EOF
{
  "language": "ru",
  "type": "quality",
  "cases": [
    {
      "form": "$INTENT",
      "base": "base/$TSV_FILE_NAME",
      "positive": "target/$INTENT.tsv",
      "negative_from_base_ratio": 1
    }
  ]
}
EOF

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/ru/granet/main.grnt"

# Append entities column to base/$TSV_FILE_NAME
"$GRANET" dataset update-entities \
    -i "$DEST_TSV"

# Optional
# Create target/$INTENT.tsv
"$GRANET" batch canonize \
    --missing \
    -g "$GRAMMAR" \
    -b "$BATCH"
