PY2TEST()

OWNER(g:megamind)

TEST_SRCS(
    test_json_util.py
    test_utils.py
)

PEERDIR(
    alice/vins/core/vins_core/utils

    contrib/python/numpy
    contrib/python/mock
    contrib/python/pytest
    contrib/python/pytest-mock
)


SIZE(SMALL)

END()
