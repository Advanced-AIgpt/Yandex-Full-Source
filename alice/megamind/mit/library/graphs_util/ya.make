PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    apphost/daemons/horizon/proto
    apphost/lib/proto_config/types_nora
    apphost/lib/python_util/conf
    contrib/python/protobuf
)

PY_SRCS(
    __init__.py
    util.py
)

END()
