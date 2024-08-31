PY3TEST()

OWNER(
    g:paskills
    penguin-diver
)

SIZE(SMALL)

TEST_SRCS(
    test_graph_storage.py
    test_nodes_storage.py
    test_ydb_table.py
)

PEERDIR(
    alice/paskills/penguinarium
    contrib/python/pytest-asyncio
    contrib/python/asynctest
)

END()
