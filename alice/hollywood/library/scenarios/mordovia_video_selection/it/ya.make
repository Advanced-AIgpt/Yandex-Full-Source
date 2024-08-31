PY3_LIBRARY()

OWNER(
    g:vh
    antonfn
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
    library/python/json
    library/python/resource
)

PY_SRCS(
    test_cases.py
    conftest.py
)

RESOURCE(
    resources/device_state.json device_state.json
)

END()

RECURSE(
    generator
    runner
)
