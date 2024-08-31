PY2TEST()

OWNER(olegtitov)

TIMEOUT(600)

SIZE(MEDIUM)

IF(AUTOCHECK)
    REQUIREMENTS(
        cpu:4
    )

    FORK_SUBTESTS()

    SPLIT_FACTOR(120)

ENDIF()


PEERDIR(
    alice/vins/apps/personal_assistant
    alice/vins/apps/crm_bot
    alice/vins/apps/personal_assistant/testing_framework
    alice/vins/apps/crm_bot/testing_framework
    alice/vins/core/test_lib
    contrib/python/requests-mock
    contrib/python/pytest-mock
    contrib/python/freezegun
)

SRCDIR(alice/vins/apps/crm_bot)

INCLUDE(${ARCADIA_ROOT}/alice/vins/apps/crm_bot/ut/resources.inc)
INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

DEPENDS(
    alice/vins/resources
    alice/vins/resources/crm_bot
)

TEST_SRCS(
    crm_bot/tests/__init__.py
    crm_bot/tests/conftest.py
    crm_bot/tests/test_functional.py
)

END()
