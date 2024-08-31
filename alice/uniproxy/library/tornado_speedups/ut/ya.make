PY3TEST()

OWNER(g:voicetech-infra)

TEST_SRCS(
    test_all.py
)

PEERDIR(
    alice/uniproxy/library/tornado_speedups
)

END()
