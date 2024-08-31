LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/logger
    alice/megamind/library/context
    alice/megamind/protos/grpc_request
    alice/protos/api/rpc
)

SRCS(
    response_builder.cpp
    scenario_request_builder.cpp
)

END()
