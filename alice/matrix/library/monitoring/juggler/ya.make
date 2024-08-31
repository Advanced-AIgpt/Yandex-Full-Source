PY3_LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    contrib/python/juggler_sdk
    contrib/python/requests

    library/python/init_log
)

PY_SRCS(
    autoconf_base.py
)

END()
