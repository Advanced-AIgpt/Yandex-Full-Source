PY3_LIBRARY()

OWNER(
    g:milab
    lvlasenkov
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
    alice/tests/integration_tests/ml
)

PY_SRCS(
    test_cases.py
)

END()

RECURSE(
    generator
    runner
)
