PY2_LIBRARY()

OWNER(olegtitov)

PEERDIR(
    alice/vins/apps/personal_assistant/testing_framework
    alice/vins/core
    alice/vins/sdk
    contrib/python/attrs
    contrib/python/PyYAML
    contrib/python/mock
)

SRCDIR(alice/vins/apps/crm_bot)

PY_SRCS(
    TOP_LEVEL
    crm_bot/testing_framework.py
)

END()
