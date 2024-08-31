OWNER(g:alice_analytics)

PY23_LIBRARY()

PY_SRCS(
    json_utils.py
    datetime_utils.py
    common.py
)

PEERDIR(
    contrib/python/pytz
    statbox/nile
)

END()

RECURSE(
    yt
    testing_utils
    auth
    nirvana
)

RECURSE_FOR_TESTS(
    tests
)
