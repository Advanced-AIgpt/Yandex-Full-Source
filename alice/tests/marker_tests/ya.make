PY3TEST()

OWNER(mihajlova)

SIZE(LARGE)

TEST_SRCS(
    conftest.py
    test.py
)

PEERDIR(
    alice/tests/library/auth
    alice/tests/library/mark
    alice/tests/library/region
    alice/tests/library/surface
    alice/tests/library/uniclient
    alice/tests/library/url
    alice/tests/library/vault
    alice/tests/library/ydb
    contrib/python/flaky
)

TAG(ya:external ya:fat ya:force_sandbox ya:not_autocheck)

REQUIREMENTS(
    sb_vault:YAV_TOKEN=file:mihajlova:yav-alice-integration-tests-token
    network:full
)

END()
