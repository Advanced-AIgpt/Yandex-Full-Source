PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(check_rm_tests py alice.tasklet.check_rm_tests.impl:CheckRmTestsImpl)

PEERDIR(
    alice/tasklet/check_rm_tests/proto
    tasklet/cli
    sandbox/projects/release_machine/client
    sandbox/projects/release_machine/components
)

END()
