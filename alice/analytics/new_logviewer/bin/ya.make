PY3_PROGRAM(logviewer)

OWNER(andreyshspb)

PEERDIR(
    contrib/python/click

    alice/analytics/new_logviewer/lib
)

PY_SRCS(
    __main__.py
)

END()
