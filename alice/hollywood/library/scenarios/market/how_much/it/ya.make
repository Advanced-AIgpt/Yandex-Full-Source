PY3_LIBRARY()

OWNER(
    g:marketinalice
)

PEERDIR(
    alice/hollywood/library/scenarios/market/testing
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
