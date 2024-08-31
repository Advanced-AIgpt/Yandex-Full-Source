PY3_PROGRAM()

OWNER(
    ran1s
    g:alice
)

PEERDIR(
    contrib/python/click

    alice/acceptance/modules/check_analytics_info/lib
)

PY_SRCS(
    __main__.py
)

END()
