PY3_PROGRAM()

OWNER(danichk)

PY_MAIN(tasklet.cli.main)

PY_SRCS(
    impl/__init__.py
)

TASKLET_REG(build_marker_tests_config py alice.tasklet.build_marker_tests_config.impl:BuildMarkerTestsConfigImpl)

PEERDIR(
    alice/tasklet/build_marker_tests_config/proto
    tasklet/cli
    tasklet/services/yav
    sandbox/projects/common/arcadia
    sandbox/projects/voicetech/resource_types
)

END()
