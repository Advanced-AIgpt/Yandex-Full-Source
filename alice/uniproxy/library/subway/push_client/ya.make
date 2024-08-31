PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    device_location_remover.py
    locations_remover.py
    push_message.py
)

PEERDIR(
    alice/uniproxy/library/settings
    alice/uniproxy/library/backends_memcached
    alice/uniproxy/library/protos
    alice/uniproxy/library/ydbs
)

END()
