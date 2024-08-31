#!/usr/bin/env bash

# https://wiki.yandex-team.ru/passport/tvm2/tvm-daemon/#opisanieformatakonfiganajsonschema
if [ -z "$BLACKBOX_ENV" ]; then
    BLACKBOX_ENV=0
fi

# $BLACKBOX_TVM_ID: https://wiki.yandex-team.ru/passport/tvm2/user-ticket/#0-opredeljaemsjasokruzhenijami
# Please, sync tvm id and alias to corresponding client const.go in case of changing
tvmtool add --secret $TVM_SECRET --src $TVM_CLIENT_ID:$TVM_CLIENT_NAME --dst $BLACKBOX_TVM_ID:$BLACKBOX_ALIAS,$TVM_CLIENT_ID:self,2010596:tuya,2016427:steelix_production,2016207:steelix_priemka,2016205:steelix_testing,2016205:steelix_testing_mimino,2016205:steelix_load_testing
tvmtool bbenv -e $BLACKBOX_ENV
tvmtool setport -p $TVM_PORT

# TODO: use native $TVMTOOL_LOCAL_AUTHTOKEN envvar
exec tvmtool -e -a $TVM_TOKEN
