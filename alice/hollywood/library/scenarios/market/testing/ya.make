PY3_LIBRARY()

OWNER(
    artemkoff
    g:marketinalice
)

PEERDIR(
    alice/hollywood/library/python/testing/integration
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
)

PY_SRCS(
    test_case.py
)

END()
