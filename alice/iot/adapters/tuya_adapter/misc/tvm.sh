#!/usr/bin/env bash

tvmtool add --secret $TVM_SECRET --src $TVM_CLIENT_ID:$TVM_CLIENT_NAME --dst 222:$BLACKBOX_ALIAS,$TVM_CLIENT_ID:self,2002490:ydb,2023123:zora,2016427:steelix_production,2016207:steelix_priemka,2016205:steelix_testing,2016205:steelix_testing_mimino,2016205:steelix_load_testing
tvmtool bbenv -e 0
tvmtool setport -p $TVM_PORT
exec tvmtool -e -a $TVM_TOKEN
