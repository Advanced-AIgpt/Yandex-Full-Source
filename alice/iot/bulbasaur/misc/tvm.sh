#!/usr/bin/env bash

# https://wiki.yandex-team.ru/passport/tvm2/tvm-daemon/#opisanieformatakonfiganajsonschema
if [ -z "$BLACKBOX_ENV" ]; then
    BLACKBOX_ENV=0
fi

# $BLACKBOX_TVM_ID: https://wiki.yandex-team.ru/passport/tvm2/user-ticket/#0-opredeljaemsjasokruzhenijami
# Please, sync tvm id and alias to corresponding client const.go in case of changing
tvmtool add --secret $TVM_SECRET --src 2009295:bulbasaur --dst 222:blackbox,2002490:ydb,2009295:self,2000252:dialogs,2000245:dialogs-testing,2000252:dialogs-priemka,2015677:cloud,2002637:quasar-backend,2002639:quasar-backend-test,2010596:tuya,2016393:remotecar,2016393:remotecar-prestable,2016393:remotecar-fake-prestable,2016061:remotecar-testing,2032560:time-machine-beta,2021514:time-machine,2016427:steelix_production,2016207:steelix_priemka,2016205:steelix_testing,2016205:steelix_testing_mimino,2016205:steelix_load_testing,2021570:memento_priemka,2021572:memento_production,132:datasync-production,2000060:datasync-testing,2023285:notificator-production,2000235:oauth-production,2034422:bass-production,2023123:zora
tvmtool bbenv -e $BLACKBOX_ENV
tvmtool setport -p 18080

# TODO: use native $TVMTOOL_LOCAL_AUTHTOKEN envvar
exec tvmtool -e -a $TVM_TOKEN
