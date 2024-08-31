#!/usr/bin/env bash
PREFIX="package(divkt-renderer):"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
ACTION_NAME='ya package';ACTION_START=`date +%s`;echo -e "$PREFIX starting $ACTION_NAME"
"$DIR"/../../ya package -r --yt-store "$DIR/pkg.json" $@
rm -v packages.json
echo -e "$PREFIX finished $ACTION_NAME in $(($(date +%s) - $ACTION_START)) seconds"
