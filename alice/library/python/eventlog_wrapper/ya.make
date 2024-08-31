PY3_LIBRARY()

OWNER(
    g:hollywood
    g:megamind
    vitvlkv
    yagafarov
)

PY_SRCS(
    eventlog_wrapper.py
    log_dump_record.py
)

PEERDIR(
    apphost/lib/proto_answers
)

END()

RECURSE_FOR_TESTS(
    tests
)
