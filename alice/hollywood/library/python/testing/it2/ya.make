PY3_LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/acceptance/modules/request_generator/lib
    alice/hollywood/library/python/testing/dump_collector
    alice/hollywood/library/python/testing/hamcrest_ext
    alice/hollywood/library/python/testing/integration
    alice/hollywood/library/python/testing/megamind_requester
    alice/library/python/testing/megamind_request
    alice/hollywood/library/python/testing/scenario_requester
    alice/megamind/library/python/testing/session_builder
    alice/megamind/protos/scenarios
    alice/memento/proto
    alice/tests/library/auth
    alice/tests/library/intent
    alice/tests/library/directives
    alice/tests/library/mark
    alice/tests/library/region
    alice/tests/library/service
    alice/tests/library/surface
    alice/tests/library/translit
    alice/tests/library/url
    alice/tests/library/uniclient
    alice/tests/library/vault
)

PY_SRCS(
    __init__.py
    alice_tests_generator.py
    alice_tests_runner.py
    conftest.py
    hamcrest.py
    input.py
    marks.py
    scenario_responses.py
    stubber.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
