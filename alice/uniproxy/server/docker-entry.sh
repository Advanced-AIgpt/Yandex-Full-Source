#!/bin/bash

#
#   Set environment type
#
ENV_TYPE=${UNIPROXY_CUSTOM_ENVIRONMENT_TYPE}
[ -z "${ENV_TYPE}" ] && ENV_TYPE=${QLOUD_ENVIRONMENT}
[ -z "${ENV_TYPE}" ] && ENV_TYPE=$(cat /etc/yandex/environment.type)
[ -z "${ENV_TYPE}" ] && ENV_TYPE=testing

cd /usr/lib/yandex/voice/uniproxy && make configure

cat << EOF > /etc/resolv.conf
nameserver ::1
nameserver 2a02:6b8:0:3400::5005
search yandex.net yandex.ru
options timeout:1 attempts:1
EOF

if [[ -z $UNIPROXY_INTEGRATION_TESTS  ]]; then
    #
    #   Execute supervisor
    #
    mkdir -p /logs
    touch /logs/current-uniproxy-eventlog
    LANG=ru_RU.UTF-8 exec /usr/bin/supervisord -n -c /etc/supervisor_qloud/supervisord.conf
else
    #
    #   Execute integration tests and report results via http server
    #
    export LANG=ru_RU.UTF-8
    echo "Sleep for sometime until uniproxy is ready to accept connections"
    sleep 10
    echo "Run tests..."
    PYTEST_BIN=py.test make test_ut >tests.out 2>&1
    RES=$?
    cat tests.out
    echo "Tests finished, start report server..."
    tests/report_tests_result.py --port=${QLOUD_HTTP_PORT:-80} --tests-output=tests.out --tests-result=$RES
fi
