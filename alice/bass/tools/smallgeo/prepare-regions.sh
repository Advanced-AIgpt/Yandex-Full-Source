#!/bin/bash

set -o errexit
set -o nounset

function loge {
    echo $@ 1>&2
}

function logi {
    echo $@
}

DEFAULT_GEOBASE="geodata5.bin"
GEOBASE="${GEOBASE:-$DEFAULT_GEOBASE}"

DEFAULT_YA="$(which ya)"
YA="${YA:-$DEFAULT_YA}"

REGIONS="regions.bin"

ROOT="$(dirname $0)"
BUILDER="$ROOT/builder/builder"

case $# in
    0) ;;
    *) loge "GEOBASE=path-to-geobase YA=path-to-ya-tool $0"
       loge
       loge "By default, GEOBASE will be set to $DEFAULT_GEOBASE."
       loge "By default, YA will be set to $YA."
       loge
       loge "This script prepares and uploads to the Sandbox list of regions."
       loge "GEOBASE should be set to the latest stable version of the libgeobase5."
       loge "If GEOBASE does not exist, the script will download the latest stable version to the current directory."
       exit 1
       ;;
esac

if test ! -x "$YA"
then
    loge "Can't find ya tool"
    loge "Exiting..."
    exit 1
fi

if test ! -r "$GEOBASE"
then
    echo "Downloading geobase..."
    curl --silent --insecure -OJ https://proxy.sandbox.yandex-team.ru/last/GEODATA5BIN_TESTING
fi

if test ! -x "$BUILDER"
then
    (
        cd "$ROOT"
        "$YA" make -j8 --checkout .
    )
fi

"./$BUILDER" --geobase "$GEOBASE" --regions "$REGIONS"
"$YA" upload "$REGIONS"
