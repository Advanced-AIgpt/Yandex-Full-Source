GO_TEST_FOR(alice/iot/bulbasaur/cmd/migration/dbmigration)

OWNER(g:alice_iot)

ENV(YDB_USE_IN_MEMORY_PDISKS=true)

ENV(YDB_TYPE=LOCAL)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

SIZE(MEDIUM)

END()
