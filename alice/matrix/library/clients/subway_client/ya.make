LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    subway_client.cpp
)

PEERDIR(
    alice/matrix/library/config
    alice/matrix/library/logging
    alice/matrix/library/metrics

    alice/uniproxy/library/protos

    apphost/lib/dns
    apphost/lib/transport

    infra/libs/outcome

    library/cpp/neh/asio
)

END()
