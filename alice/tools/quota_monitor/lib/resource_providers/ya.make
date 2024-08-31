PY3_LIBRARY()
OWNER(g:voicetech-infra)
PY_SRCS(
    base_resource_provider.py
    yp_provider.py
)
PEERDIR(
    yp/python/client
    alice/tools/version_alerts/lib
    contrib/python/aiohttp
)

END()
