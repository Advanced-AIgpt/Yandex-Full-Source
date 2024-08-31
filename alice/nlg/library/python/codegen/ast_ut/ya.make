PY2TEST()

OWNER(g:alice)

PEERDIR(
    alice/nlg/library/python/codegen
    alice/nlg/library/python/codegen_tool
)

PY_SRCS(
    common.py
)

TEST_SRCS(
    test_keyset_producer.py
    test_transformer.py
)

DATA(
    arcadia/alice/nlg/library/python/codegen/ast_ut/nlg_templates
)

END()
