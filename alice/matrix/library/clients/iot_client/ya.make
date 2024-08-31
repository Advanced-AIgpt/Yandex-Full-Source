LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    iot_client.cpp
)

PEERDIR(
    alice/matrix/library/clients/tvm_client
    alice/matrix/library/config
    alice/matrix/library/logging
    alice/matrix/library/metrics

    alice/megamind/protos/common

    apphost/lib/dns
    apphost/lib/transport

    infra/libs/outcome

    library/cpp/neh/asio
)

END()
