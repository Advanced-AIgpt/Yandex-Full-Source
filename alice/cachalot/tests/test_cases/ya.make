PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PEERDIR(
    alice/cachalot/client
    alice/cuttlefish/library/protos
    library/python/codecs
)

PY_SRCS(
    __init__.py
    activation.py
    cache.py
    util.py
    yabio_context.py
)

END()
