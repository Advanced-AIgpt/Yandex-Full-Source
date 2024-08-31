PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/python/test_utils
    contrib/python/tornado/tornado-6
)

PY_SRCS(
    __init__.py
    http_utils.py
)

END()
