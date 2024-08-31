#!/usr/bin/env bash

echo `dirname $0`/local-conf.json
SELF_SECRET=`cat $HOME/.tvm/2016205.secret` TVMTOOL_LOCAL_AUTHTOKEN=token_for_local_interaction_1234 tvmtool --config `dirname $0`/local-conf.json
