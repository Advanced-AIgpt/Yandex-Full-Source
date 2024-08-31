LIBRARY()

OWNER(
    deemonasd
    g:hollywood
)

PEERDIR(
    alice/hollywood/protos
    alice/hollywood/library/base_scenario
    alice/library/json
    alice/library/logger
    alice/library/network

    apphost/lib/proto_answers
)

SRCS(
    http_proxy.cpp
    request_meta_provider.cpp
)

END()
