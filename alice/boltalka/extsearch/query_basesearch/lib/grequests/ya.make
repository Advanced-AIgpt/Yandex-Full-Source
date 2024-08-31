PY3_LIBRARY()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    contrib/python/grequests
    yt/python/client
    alice/boltalka/extsearch/query_basesearch/lib
)

END()