PY3TEST()

OWNER(g:wonderlogs)

PEERDIR(
    contrib/python/requests-mock

    alice/wonderlogs/daily/error_nottifier/lib
)

TEST_SRCS(
     config_parser.py
     juggler.py
     reporter.py
)

END()
