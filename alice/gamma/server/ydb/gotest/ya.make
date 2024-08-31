GO_TEST_FOR(alice/gamma/server/ydb)

SIZE(MEDIUM)

OWNER(
    g-kostin
    g:alice
)

ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

TAG(ya:external)

END()
