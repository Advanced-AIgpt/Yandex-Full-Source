PY3TEST()

OWNER(
    g:voicetech-infra
)

FORK_SUBTESTS()

TEST_SRCS(
    sessionlog_ut.py
    async_file_logger_ut.py
    log_filter_ut.py
)

PEERDIR(
    alice/uniproxy/library/logging
    alice/uniproxy/library/events
    alice/uniproxy/library/extlog
    alice/uniproxy/library/extlog/mocks
    alice/uniproxy/library/testing

    contrib/python/tornado/tornado-4
)

END()

