PY3TEST()

OWNER(
    g:voicetech-infra
)

FORK_SUBTESTS()

TEST_SRCS(
    test_yp_provider.py
)

PEERDIR(
    alice/tools/quota_monitor/lib/resource_providers
)

END()
