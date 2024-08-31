#!/usr/bin/env bash

# https://wiki.yandex-team.ru/passport/tvm2/tvm-daemon/#opisanieformatakonfiganajsonschema
if [ -z "$BLACKBOX_ENV" ]; then
    BLACKBOX_ENV=0
fi

# $BLACKBOX_TVM_ID: https://wiki.yandex-team.ru/passport/tvm2/user-ticket/#0-opredeljaemsjasokruzhenijami
# Please, sync tvm id and alias to corresponding client const.go in case of changing
tvmtool add --secret $TVM_SECRET --src 2036308:xiaomi --dst 2036308:self,2023123:zora
tvmtool bbenv -e $BLACKBOX_ENV
tvmtool setport -p $TVM_PORT

# TODO: use native $TVMTOOL_LOCAL_AUTHTOKEN envvar
exec tvmtool -e -a $TVM_TOKEN
