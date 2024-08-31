PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    global_counter_ut.py
    delivery_ut.py
    uniproxy_ut.py
)

FORK_TEST_FILES()

PEERDIR(
    alice/uniproxy/library/global_counter
)

END()
