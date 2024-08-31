PY3_LIBRARY()

OWNER(
    tolyandex
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
)

PY_SRCS(tests_data.py)

END()

RECURSE(
    generator
    runner
)
