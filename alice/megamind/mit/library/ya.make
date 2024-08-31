PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/megamind/mit/library/generator
    alice/megamind/mit/library/graphs_util
    alice/megamind/mit/library/runner
    alice/megamind/mit/library/stubber
    alice/megamind/mit/library/util
    alice/tests/library/service
    library/python/testing/yatest_common
)

PY_SRCS(
    __init__.py
    conftest.py
)

END()

RECURSE(
    apphost
    common
    generator
    graphs_util
    proto
    request_builder
    requester
    response
    runner
    stubber
    util
)

RECURSE_FOR_TESTS(
    mypy
)
