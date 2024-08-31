PY3_LIBRARY()

OWNER(
    tolyandex
    vitvlkv
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
)

PY_SRCS(
    apps_fixlist_tests_data.py
    test_cases_push.py
    tests_data.py
)

END()

RECURSE(
    generator
    runner
)
