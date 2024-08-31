PY2TEST()

OWNER(g:alice)

TIMEOUT(600)

SIZE(MEDIUM)

REQUIREMENTS(network:full ram:12)

INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

DEPENDS(alice/vins/resources)

PEERDIR(
    alice/vins/api
    alice/memento/proto
    alice/protos/data/scenario/reminders
    contrib/python/mongomock
    contrib/python/pytest-mock
    contrib/python/mock
    contrib/python/requests-mock
)

SRCDIR(alice/vins/api)

TEST_SRCS(
    vins_api/external_skill/tests.py
    vins_api/speechkit/tests/__init__.py
    vins_api/speechkit/tests/test_session.py
    vins_api/speechkit/tests/test_api.py
    vins_api/speechkit/tests/test_common.py
    vins_api/speechkit/tests/test_navi.py
    vins_api/speechkit/tests/test_validation.py
    vins_api/speechkit/tests/test_protocol.py
    vins_api/speechkit/tests/test_qa.py
    vins_api/speechkit/tests/conftest.py
    vins_api/webim/tests/__init__.py
    vins_api/webim/tests/conftest.py
    vins_api/webim/tests/test_api.py
    vins_api/webim/tests/test_api_v2.py
    vins_api/webim/tests/test_api_ocrm.py
    vins_api/webim/tests/test_api_ocrm_v2.py
    vins_api/webim/tests/test_experiments.py
    vins_api/webim/tests/test_tsum_trace.py
)

END()
