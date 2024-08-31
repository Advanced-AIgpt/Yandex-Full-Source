PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    common.py
    spotter_features.py
    storage_cachalot.py
    storage_null.py
)

PEERDIR(
    alice/cachalot/client
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/settings
    alice/uniproxy/library/logging
)

END()
