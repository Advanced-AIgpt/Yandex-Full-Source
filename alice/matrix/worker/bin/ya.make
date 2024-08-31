PROGRAM(matrix_worker)

OWNER(
    g:matrix
)

SRCS(
    main.cpp
)

RESOURCE(
    alice/matrix/worker/bin/config.json /proto_config/config.json
)

PEERDIR(
    alice/matrix/worker/library/config
    alice/matrix/worker/library/services/common_context
    alice/matrix/worker/library/services/worker

    alice/matrix/library/daemon
    alice/matrix/library/services/metrics
)

END()
