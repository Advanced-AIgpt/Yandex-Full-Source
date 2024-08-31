PY3TEST()


OWNER(
    g:voicetech-infra
)


TEST_SRCS(
    subway_pull_client_test.py
    subway_push_client_test.py
    mocks.py
)

PEERDIR(
    alice/uniproxy/library/subway/pull_client
    alice/uniproxy/library/subway/push_client
    alice/uniproxy/library/protos
    alice/uniproxy/library/testing
)

DEPENDS(
    alice/uniproxy/bin/uniproxy-subway
)

SIZE(MEDIUM)

END()
