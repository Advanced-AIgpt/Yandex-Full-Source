PY3_PROGRAM()

OWNER(g:voicetech-infra)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/tools/version_alerts/lib
    alice/tools/quota_monitor/lib
    contrib/python/aiohttp
)
END()
RECURSE_FOR_TESTS(tests)
