UNITTEST_FOR(alice/nlu/granet/lib/utils)

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    parallel_processor_ut.cpp
    rle_array_ut.cpp
    string_utils_ut.cpp
    variadic_format_ut.cpp
)

PEERDIR(
    alice/nlu/libs/ut_utils
    library/cpp/iterator
)

END()
