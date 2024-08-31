PY3TEST()

OWNER(g:alice)

SIZE(SMALL)

TEST_SRCS(
    conftest.py
    test_device_state.py
    test_experiments.py
    test_oauth.py
    test_region.py
)

PEERDIR(
    alice/tests/library/auth
    alice/tests/library/mark
    contrib/python/pytest
)

END()
