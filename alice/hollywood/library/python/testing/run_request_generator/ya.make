PY3_LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/acceptance/modules/request_generator/lib
    alice/hollywood/library/python/testing/dump_collector
    alice/hollywood/library/python/testing/integration
    alice/hollywood/library/python/testing/megamind_requester
    alice/megamind/fixture
    alice/megamind/protos/scenarios
    alice/tests/library/surface
    alice/tests/library/uniclient
    alice/tests/library/vins_response
)

PY_SRCS(
    conftest.py
    run_request_generator.py
)

END()
