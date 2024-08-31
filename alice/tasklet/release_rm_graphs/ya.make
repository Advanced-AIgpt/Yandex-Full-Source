PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(release_rm_graphs py alice.tasklet.release_rm_graphs.impl:ReleaseRmGraphsImpl)

PEERDIR(
    alice/tasklet/release_rm_graphs/proto
    tasklet/cli
    sandbox/projects/release_machine/client
    sandbox/projects/release_machine/components
    sandbox/projects/release_machine/core
    sandbox/projects/common/testenv_client
    tasklet/services/yav
)

END()
