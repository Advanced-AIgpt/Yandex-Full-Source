PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    contrib/python/python-telegram-bot
    alice/vins/core
)

PY_SRCS(
    TOP_LEVEL
    vins_sdk/__init__.py
    vins_sdk/app.py
    vins_sdk/connectors.py
)

END()

RECURSE_FOR_TESTS(ut)
