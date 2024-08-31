PY2TEST()

OWNER(g:alice)

PEERDIR(
    alice/nlg/library/python/codegen_tool
)

DATA(
    arcadia/alice/nlg/library/python/codegen_tool/ut/nlg/tests_1.nlg
)

TEST_SRCS(
    test_tools.py
)

END()
