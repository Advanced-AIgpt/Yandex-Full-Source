UNITTEST_FOR(alice/bass/libs/ydb_helpers)

OWNER(g:bass)

SIZE(MEDIUM)

PEERDIR(
    alice/bass/libs/ydb_helpers/ut_protos
    alice/bass/libs/logging_v2
)

ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

SRCS(
    table_ut.cpp
)

END()
