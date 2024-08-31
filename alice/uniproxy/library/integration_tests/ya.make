PY3TEST()

OWNER(
    g:voicetech-infra
)

FORK_TEST_FILES()

TEST_SRCS(
    blackbox_ut.py
    tvm2_ut.py
)

PEERDIR(
    alice/uniproxy/library/auth
    alice/uniproxy/library/backends_tts
    alice/uniproxy/library/backends_memcached
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
    alice/uniproxy/library/testing
    alice/uniproxy/library/utils

    contrib/python/tornado/tornado-4

    library/python/deprecated/ticket_parser2
)

SET(PWD "arcadia/alice/uniproxy/library/integration_tests")
DATA(
    ${PWD}/test.token
)

END()
