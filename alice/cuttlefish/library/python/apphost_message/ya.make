PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    apphost/lib/grpc/protos
    library/python/codecs
)

PY_SRCS(
    __init__.py
    constants.py
    messages.py
    packing.py
)

END()
