PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(convert_version_to_branch_tag py alice.tasklet.convert_version_to_branch_tag.impl:ConvertVersionToBranchTagImpl)

PEERDIR(
    alice/tasklet/convert_version_to_branch_tag/proto
    tasklet/cli
)

END()
