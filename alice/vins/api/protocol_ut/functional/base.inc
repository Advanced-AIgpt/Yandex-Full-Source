PY2TEST()

OWNER(g:alice)

SIZE(MEDIUM)

REQUIREMENTS(
    network:full
    cpu:4
    ram:16
)

FORK_SUBTESTS()

IF(AUTOCHECK)
    TIMEOUT(600)

    SPLIT_FACTOR(20)

ELSE()
    TIMEOUT(10000)

    SPLIT_FACTOR(4)

    TAG(ya:not_autocheck)

ENDIF()

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

SRCDIR(alice/vins/api)

RESOURCE(
    app_presets.yaml /load_app_infos/app_presets.yaml
)

TEST_SRCS(
    vins_api/speechkit/test_protocol/__init__.py
    vins_api/speechkit/test_protocol/conftest.py
    vins_api/speechkit/test_protocol/test_functional.py
)

END()
