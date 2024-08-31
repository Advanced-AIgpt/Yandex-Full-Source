#!/usr/bin/env bash
curl --silent --insecure -o mdsdelete https://proxy.sandbox.yandex-team.ru/3064746424
chmod 777 ./mdsdelete
export YT_PROXY=hahn

DATE=$(echo ${1})
TABLE_SUFFIX=$(echo ${2})
COLUMN=$(echo ${3})

FILENAME="${TABLE_SUFFIX}.${DATE}.${COLUMN}.txt"

THIS_DIR=$(dirname $(readlink -f $0))
#   CHECK FOR YQL BINARY
YA_BIN=$(which ya)
[ -z ${YA_BIN} ] && YA_BIN="${THIS_DIR}/ya"
[ ! -x ${YA_BIN} ] && YA_BIN="$(dirname ${THIS_DIR})/ya"
[ ! -x ${YA_BIN} ] && exit 13




echo $DATE
echo $FILENAME
TABLE="//home/voice-speechbase/uniproxy/$TABLE_SUFFIX/$DATE{$COLUMN}"
echo $TABLE
$YA_BIN tool yt read $TABLE --format dsv | sed -e "s/^$COLUMN=//" > $FILENAME
./mdsdelete -file $FILENAME -jobs 700
