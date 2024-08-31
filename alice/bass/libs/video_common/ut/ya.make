UNITTEST_FOR(alice/bass/libs/video_common)

OWNER(g:bass)

SIZE(MEDIUM)
SPLIT_FACTOR(30)
FORK_SUBTESTS()

ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    alice/bass/libs/video_common/ut/protos
    alice/bass/libs/ut_helpers
    alice/bass/libs/source_request
)

SRCS(
    content_db_ut.cpp
    content_db_check_utils_ut.cpp
    has_good_result_ut.cpp
    item_selection_ut.cpp
    kinopoisk_recommendations_ut.cpp
    kinopoisk_utils_ut.cpp
    show_or_gallery_ut.cpp
    universal_api_utils_ut.cpp
    utils_ut.cpp
    yavideo_utils_ut.cpp
    youtube_utils_ut.cpp
)

END()

RECURSE(protos)
