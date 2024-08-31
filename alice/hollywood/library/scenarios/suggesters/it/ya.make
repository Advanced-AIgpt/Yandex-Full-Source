PY3_LIBRARY()

OWNER(
    dan-anastasev
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
)

PY_SRCS(
    test_cases_games.py
    test_cases_movies.py
)

END()

RECURSE_FOR_TESTS(
    runner
    generator
)
