PY3_PROGRAM()

OWNER(g:voicetech-infra)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/aiohttp
    alice/tools/version_alerts/lib
    alice/tools/version_alerts/parsers
)

END()
