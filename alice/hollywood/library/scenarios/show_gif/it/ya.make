PY3_LIBRARY()

OWNER(
    jan-fazli
    g:alice
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
)

PY_SRCS(test_cases.py)

END()

RECURSE(
    generator
    runner
)