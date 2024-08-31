PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(remove_old_beta py alice.tasklet.remove_old_beta.impl:RemoveOldBetaImpl)

PEERDIR(
    alice/tasklet/remove_old_beta/proto
    search/priemka/yappy/services
    search/martylib/core/exceptions
    tasklet/cli
    tasklet/services/yav
)

END()
