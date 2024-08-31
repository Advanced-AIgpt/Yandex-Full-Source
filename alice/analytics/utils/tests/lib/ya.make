OWNER(g:alice_analytics)

PY23_LIBRARY()

PEERDIR(
    alice/analytics/utils
    alice/analytics/utils/yt/relative_lib
)

TEST_SRCS(
    test_json_utils.py
    test_datetime_utils.py
    test_basket_common.py
)


END()
