PY3TEST()

OWNER(andreyshspb)

PEERDIR(
    contrib/python/pytest
    contrib/python/freezegun

    alice/analytics/new_logviewer/lib
)

TEST_SRCS(
    app_checker.py
    search_checker.py
)

END()
