PY3_LIBRARY()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    nirvana/valhalla/src
    alice/boltalka/extsearch/query_basesearch/lib/grequests
)

END()