PY3_LIBRARY()

OWNER(g:wonderlogs)

PEERDIR(
    alice/wonderlogs/protos
)

PY_SRCS(
    config_parser.py
    juggler.py
    reporter.py
)

END()

RECURSE_FOR_TESTS(ut)
