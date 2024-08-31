PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(generate_uniproxy_url_with_srcrwr py alice.tasklet.generate_uniproxy_url_with_srcrwr.impl:GenerateUniproxyUrlWithSrcrwrImpl)

PEERDIR(
    alice/tasklet/generate_uniproxy_url_with_srcrwr/proto
    tasklet/cli
)

END()
