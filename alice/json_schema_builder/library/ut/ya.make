PY3TEST()

OWNER(g:alice)

PEERDIR(
    alice/json_schema_builder/library
)

TEST_SRCS(
    test_nodes.py
    test_parser.py
    test_ref.py
    test_util.py
)

END()
