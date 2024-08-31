#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(dirname "$(readlink -f "$BASH_SOURCE")")"
[[ "$ARCADIA" ]] || { ARCADIA="${SCRIPT_DIR%/alice/nlu/granet*}"; }
pushd "$SCRIPT_DIR" >> /dev/null

echo "Building granet tool..."
GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
ya make -r --checkout $(dirname "$GRANET")

echo "Updating entities..."
$GRANET batch update-entities -b "$SCRIPT_DIR" "$@"

echo "Done"

popd >> /dev/null
