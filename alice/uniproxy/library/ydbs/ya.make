PY3_LIBRARY()

OWNER(
    g:matrix
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    device_locator.py
)

PEERDIR(
    alice/megamind/protos/scenarios
    alice/protos/api/notificator

    alice/uniproxy/library/backends_common
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/settings

    library/python/cityhash
)

END()

