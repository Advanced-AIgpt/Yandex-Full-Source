PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/vins/apps/personal_assistant
    alice/vins/core
    alice/vins/sdk
    contrib/python/attrs
    contrib/python/PyYAML
    contrib/python/mock
)

SRCDIR(alice/vins/apps/personal_assistant)

PY_SRCS(
    TOP_LEVEL
    personal_assistant/testing_framework.py
)

END()
