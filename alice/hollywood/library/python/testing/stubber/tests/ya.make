PY3TEST()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/hollywood/library/python/testing/stubber
    contrib/python/requests-mock
)

TEST_SRCS(
    conftest.py
    test_filter_headers.py
    test_static_stubber.py
    test_stubber_server.py
)

DATA(
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_duplicates
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_get
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_get_frozen
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_post
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_post_json
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_post_json_not_sorted
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_put
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_static_stubber
    arcadia/alice/hollywood/library/python/testing/stubber/tests/data_unused_stubs
)

END()
