PY3_PROGRAM(lb_alert_creator)

OWNER(g:wonderlogs)

PEERDIR(
    alice/wonderlogs/tools/lb_alert_creator/lib

    contrib/python/click
)

PY_SRCS(__main__.py)

END()
