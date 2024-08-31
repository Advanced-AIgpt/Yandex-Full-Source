OWNER(
    g-kostin
    g:alice
)

PY3TEST()

TEST_SRCS(
    test_context.py
    test_matcher.py
    test_parser.py
    test_sdk.py
)

PEERDIR(
    contrib/libs/grpc

    alice/gamma/sdk/api
    alice/gamma/sdk/python/gamma_sdk
    alice/gamma/sdk/python/gamma_sdk/sdk
    alice/gamma/sdk/python/gamma_sdk/testing
)

END()
