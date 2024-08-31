UNITTEST_FOR(alice/bass/libs/ydb_kv)

OWNER(g:bass)

SIZE(MEDIUM)
SPLIT_FACTOR(10)
FORK_SUBTESTS()

ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(alice/bass/libs/ydb_helpers)

SRC(kv_ut.cpp)

END()
