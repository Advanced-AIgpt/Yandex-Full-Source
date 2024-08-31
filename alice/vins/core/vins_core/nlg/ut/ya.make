PY2TEST()

OWNER(g:alice)

PEERDIR(
    alice/vins/core/vins_core/nlg
    alice/vins/core/vins_core/utils

    contrib/python/dateutil
    contrib/python/freezegun
    contrib/python/pytest
    contrib/python/pytest-mock
    contrib/python/pytz
    contrib/python/requests-mock
)

TEST_SRCS(
     test_nlg_filters.py
)

RESOURCE_FILES(
    data/datetime_raw_examples.json
)


SIZE(SMALL)

END()
