PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/cachalot/client
    alice/rtlog/client/python/lib
    alice/uniproxy/library/settings
    alice/uniproxy/library/global_counter

    library/python/cityhash
)

PY_SRCS(
    __init__.py
)

END()
