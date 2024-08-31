LIBRARY()

OWNER(
    g:alice_downloaders
)

SRCS(
    request.cpp
)

PEERDIR(
    alice/library/json
    alice/uniproxy/mapper/fetcher/lib/protos
    alice/uniproxy/mapper/library/logging
    alice/uniproxy/mapper/uniproxy_client/lib
    library/cpp/retry
    library/cpp/string_utils/base64
)

END()
