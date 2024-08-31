LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    tvm_client.cpp
)

PEERDIR(
    alice/matrix/library/config
    alice/matrix/library/logging
    alice/matrix/library/metrics

    infra/libs/outcome

    library/cpp/tvmauth/client
)

END()

RECURSE_FOR_TESTS(
    ut
)
