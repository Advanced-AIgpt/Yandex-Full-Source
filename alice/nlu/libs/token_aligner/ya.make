LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    aligner.cpp
    alignment.cpp
)

PEERDIR(
    alice/nlu/libs/interval
    library/cpp/string_utils/levenshtein_diff
)

END()

RECURSE_FOR_TESTS(ut)
