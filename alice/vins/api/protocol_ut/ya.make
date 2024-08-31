PY2TEST()

OWNER(g:alice)

TIMEOUT(10000)

SIZE(MEDIUM)

REQUIREMENTS(
    network:full
    cpu:4
)

FORK_SUBTESTS()

SPLIT_FACTOR(4)

INCLUDE(${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/ut/resources.inc)
INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

DEPENDS(alice/vins/resources)

PEERDIR(
    alice/vins/api
    alice/vins/apps/personal_assistant
    alice/vins/apps/personal_assistant/testing_framework
    alice/vins/core/test_lib
    contrib/python/pytest-mock
    contrib/python/mock
)

TAG(
    ya:manual
)

SRCDIR(alice/vins/api)

TEST_SRCS(
    vins_api/speechkit/test_protocol/__init__.py
    vins_api/speechkit/test_protocol/conftest.py
    vins_api/speechkit/test_protocol/test_integration.py
)

END()
