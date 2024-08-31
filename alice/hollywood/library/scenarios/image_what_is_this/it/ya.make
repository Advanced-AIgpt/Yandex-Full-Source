PY3_LIBRARY()

OWNER(
    polushkin
    g:cv-dev
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
    alice/hollywood/library/scenarios/image_what_is_this/proto
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
