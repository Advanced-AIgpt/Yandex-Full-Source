PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(create_new_rm_branch py alice.tasklet.create_new_rm_branch.impl:CreateNewRmBranchImpl)

PEERDIR(
    alice/tasklet/create_new_rm_branch/proto
    tasklet/cli
    sandbox/projects/release_machine/client
    sandbox/projects/release_machine/components
    tasklet/services/ci
)

END()
