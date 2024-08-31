PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(run_yasm_alert_generator py alice.tasklet.run_yasm_alert_generator.impl:RunYasmAlertGeneratorImpl)

PEERDIR(
    alice/tasklet/run_yasm_alert_generator/proto
    tasklet/cli
    alice/tools/yasm/client/library
)

END()
