PY3_LIBRARY()
OWNER(g:voicetech-infra)

PY_SRCS(
    __init__.py
    speedups.pyx
)

PEERDIR(
    contrib/python/tornado/tornado-4
    contrib/python/python-rapidjson
)

END()

RECURSE_FOR_TESTS(ut)
