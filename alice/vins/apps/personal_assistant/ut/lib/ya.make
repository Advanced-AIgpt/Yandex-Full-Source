PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/vins/apps/personal_assistant
    alice/vins/apps/personal_assistant/testing_framework
    alice/vins/core/test_lib
    contrib/python/mock
    contrib/python/requests-mock
    contrib/python/pytest
    contrib/python/pytest-mock
    contrib/python/freezegun
)

INCLUDE(${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/ut/resources.inc)

SRCDIR(alice/vins/apps/personal_assistant)

TEST_SRCS(
    personal_assistant/tests/conftest.py
    personal_assistant/tests/consts.py
    personal_assistant/tests/__init__.py
)

END()
