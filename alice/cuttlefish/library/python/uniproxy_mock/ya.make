PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-6
    alice/cuttlefish/library/python/test_utils_with_tornado
)

PY_SRCS(__init__.py)

END()
