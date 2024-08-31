PY3TEST()

OWNER(
    g:paskills
    penguin-diver
)

SIZE(SMALL)

TEST_SRCS(
    conftest.py
    test_graph_storage.py
    test_nodes_storage.py
    test_ydb_table.py
    test_ydb_utils.py
)

PEERDIR(
    alice/paskills/penguinarium
    contrib/python/pytest-asyncio
)

ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

FORK_TESTS()
FORK_TEST_FILES()

END()
