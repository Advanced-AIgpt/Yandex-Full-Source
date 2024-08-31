PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/event_patcher
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
    alice/uniproxy/library/uaas_mapper
    alice/uniproxy/library/utils
    contrib/python/tornado/tornado-4
    laas/lib/ip_properties/proto
)

PY_SRCS(
    __init__.py
)

END()
