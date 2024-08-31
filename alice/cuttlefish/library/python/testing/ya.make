PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/protos
    library/python/codecs
    voicetech/asr/engine/proto_api
    apphost/lib/grpc/protos
    apphost/lib/proto_answers
)

PY_SRCS(
    __init__.py
    constants.py
    checks.py
    items.py
    utils.py
)

END()
