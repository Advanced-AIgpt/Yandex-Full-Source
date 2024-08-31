PY3_LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
)

PY_SRCS(
    conftest.py
    test_cases.py
)

END()

RECURSE(
    generator
    runner
)
