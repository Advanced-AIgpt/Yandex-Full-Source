#!/bin/sh
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
"$DIR"/../../../../ya package "$DIR/pkg.json" --docker --docker-repository=paskills $@
