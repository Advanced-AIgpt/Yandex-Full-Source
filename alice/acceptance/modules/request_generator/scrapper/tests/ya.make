PY3TEST()

OWNER(
    g:alice-downloaders
)

TEST_SRCS(
    test_lib/test_io.py
)

PEERDIR(
    contrib/python/click
    contrib/python/pytest
    contrib/python/pytest-mock
    contrib/python/freezegun

    alice/acceptance/modules/request_generator/lib
    alice/acceptance/modules/request_generator/scrapper/lib
)

SIZE(SMALL)

ENV(TEST_LOGNAME=pytest)
ENV(TEST_FQDN=pytest.local)

END()
