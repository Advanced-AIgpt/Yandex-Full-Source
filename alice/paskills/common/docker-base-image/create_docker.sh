#!/bin/sh
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REVISION=`arc log -n 1 trunk | grep 'revision' | awk '{print substr($0,11)}'`
#
docker build --no-cache -t "registry.yandex.net/paskills/jvm-base-image:$REVISION" "$DIR/."
