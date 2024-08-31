UNITTEST_FOR(alice/bass/libs/video_tools/quick_updater)

OWNER(g:bass)

SIZE(MEDIUM)

ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    alice/bass/libs/request
    alice/bass/libs/ut_helpers
)

SRCS(quick_updater_utils_ut.cpp)

END()
