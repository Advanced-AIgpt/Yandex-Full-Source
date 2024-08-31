LIBRARY()

OWNER(g:cards_serivce)

PEERDIR(
    kernel/lemmer
    quality/trailer/trailer_common
)

SRCS(
    parsed_sequence.cpp
    parsed_user_phrase.cpp
    stopwords.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
