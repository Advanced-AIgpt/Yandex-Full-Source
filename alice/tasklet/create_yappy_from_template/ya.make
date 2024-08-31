PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(create_yappy_from_template py alice.tasklet.create_yappy_from_template.impl:CreateBetaFromTemplateImpl)

PEERDIR(
    alice/tasklet/create_yappy_from_template/proto
    search/priemka/yappy/services
    tasklet/cli
    tasklet/services/yav
    infra/nanny/yp_lite_api/py_stubs
    infra/nanny/nanny_rpc_client
)

END()
