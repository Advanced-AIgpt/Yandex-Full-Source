YT_UNITTEST()

OWNER(g:wonderlogs)

SIZE(medium)

INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe.inc)

SRCS(
    utils_ut.cpp
)

PEERDIR(
    alice/wonderlogs/library/common
    alice/wonderlogs/library/yt

    mapreduce/yt/tests/yt_unittest_lib
    mapreduce/yt/tests/yt_unittest_main
)

END()
