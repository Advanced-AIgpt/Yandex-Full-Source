PROGRAM()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/logging
    library/cpp/terminate_handler
    apphost/lib/grpc/client
    apphost/lib/grpc/json
    apphost/lib/grpc/protos
)

SRCS(
    apphost_session.h
    apphost_session.cpp
    main.cpp
)

END()
