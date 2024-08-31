PY3_LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
    alice/library/python/testing/auth
)

PY_SRCS(test_cases.py)

END()

RECURSE(
    generator
    runner
)
