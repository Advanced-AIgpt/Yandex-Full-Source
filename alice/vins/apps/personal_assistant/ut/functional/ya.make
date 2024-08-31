PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/vins/apps/personal_assistant/ut/lib
)

SRCDIR(alice/vins/apps/personal_assistant)
TEST_SRCS(
    personal_assistant/tests/test_functional.py
)

END()
