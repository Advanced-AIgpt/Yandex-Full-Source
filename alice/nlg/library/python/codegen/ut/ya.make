PY2TEST()

OWNER(g:alice)

PEERDIR(
    alice/nlg/library/python/codegen
    alice/nlg/library/python/codegen_tool
)

TEST_SRCS(
    test_ast_visitor.py
    test_call.py
    test_failed_templates.py
)

DATA(
    arcadia/alice/nlg/library/python/codegen/ut/failed_templates
)

END()
