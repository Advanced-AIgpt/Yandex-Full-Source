LIBRARY()

OWNER(
    g:alice
    tolyandex
)

PEERDIR(
    alice/library/parsed_user_phrase
    alice/nlu/libs/request_normalizer
    alice/library/response_similarity/proto
)

SRCS(
    response_similarity.cpp
)

END()

RECURSE_FOR_TESTS(ut)
