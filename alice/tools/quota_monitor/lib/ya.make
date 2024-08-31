PY3_LIBRARY()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/tools/quota_monitor/lib/resource_providers
)

PY_SRCS(
    collector.py
)

END()
