PY3_LIBRARY()

OWNER(
    g:hollywood
    khr2
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
)

PY_SRCS(
    tests_data_push.py
    tests_data.py
)

END()

RECURSE(
    generator
    runner
)
