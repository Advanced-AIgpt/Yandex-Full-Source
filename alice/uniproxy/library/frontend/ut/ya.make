PY3TEST()

OWNER(g:voicetech-infra)

TEST_SRCS(
    test_as_single_binary.py
)

PEERDIR(
    alice/uniproxy/library/frontend
    alice/uniproxy/library/global_counter
)

END()
