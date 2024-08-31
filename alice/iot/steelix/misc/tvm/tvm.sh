#!/usr/bin/env bash

if [ "${ENV_TYPE}" == "BETA" ]; then
    exec tvmtool -e -c /etc/tvmtool/beta-conf.json -a $TVM_TOKEN
elif [ "${ENV_TYPE}" == "TEST" ]; then
    exec tvmtool -e -c /etc/tvmtool/test-conf.json -a $TVM_TOKEN
elif [ "${ENV_TYPE}" == "STRESS" ]; then
    exec tvmtool -e -c /etc/tvmtool/stress-conf.json -a $TVM_TOKEN
else
    exec tvmtool -e -c /etc/tvmtool/production-conf.json -a $TVM_TOKEN
fi
