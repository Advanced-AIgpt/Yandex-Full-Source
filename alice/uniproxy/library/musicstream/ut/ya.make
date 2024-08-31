PY3TEST()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/musicstream
    alice/uniproxy/library/global_counter
)

TEST_SRCS(
    test_musicstream.py
)

END()
