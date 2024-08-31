PY3_PROGRAM(notification_creator)

OWNER(
    g:mediaalice
)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    alice/notification_creator/lib
    contrib/python/gunicorn
)

END()
