PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(uniproxy_regress_tests py alice.tasklet.uniproxy_regress_tests.impl:UniproxyRegressTestsImpl)

PEERDIR(
    alice/tasklet/uniproxy_regress_tests/proto
    tasklet/cli
    alice/tools/yasm/client/library
    library/python/startrek_python_client
    tasklet/services/yav
    tasklet/services/ci
    contrib/python/aiohttp
)

END()
