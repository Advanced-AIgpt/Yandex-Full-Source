PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PEERDIR(
    alice/cachalot/client
    alice/cuttlefish/tests/common
    library/python/resource
)

PY_SRCS(
    __init__.py
)

RESOURCE(
    alice/cachalot/library/config/cachalot-activation.json /cachalot-activation.json
    alice/cachalot/library/config/cachalot-context.json /cachalot-context.json
    alice/cachalot/library/config/cachalot-gdpr.json /cachalot-gdpr.json
    alice/cachalot/library/config/cachalot-mm.json /cachalot-mm.json
    alice/cachalot/library/config/cachalot-tts.json /cachalot-tts.json
)

END()
