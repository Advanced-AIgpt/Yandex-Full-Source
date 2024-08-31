PROGRAM(matrix_scheduler)

OWNER(
    g:matrix
)

SRCS(
    main.cpp
)

RESOURCE(
    alice/matrix/scheduler/bin/config.json /proto_config/config.json
)

PEERDIR(
    alice/matrix/scheduler/library/config
    alice/matrix/scheduler/library/services/common_context
    alice/matrix/scheduler/library/services/scheduler

    alice/matrix/library/daemon
    alice/matrix/library/services/metrics
)

END()
