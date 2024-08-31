LIBRARY()

OWNER(
    nkodosov
    g:hollywood
)

PEERDIR(
    alice/cuttlefish/library/protos
    alice/hollywood/library/hw_service_context
    alice/megamind/protos/scenarios
)

SRCS(
    rpc_request.cpp
    rpc_response.cpp
)

END()

RECURSE_FOR_TESTS(ut)
