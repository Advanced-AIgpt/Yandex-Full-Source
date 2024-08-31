LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    collect_from_context.cpp
    dictionary.cpp
    finder.cpp
    finder_impl.cpp
    token.cpp
)

PEERDIR(
    alice/nlu/granet/lib/lang
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/utils
    alice/nlu/libs/request_normalizer
    alice/nlu/libs/token_aligner
    alice/library/json
    alice/library/video_common
    library/cpp/iterator
    library/cpp/string_utils/base64
    library/cpp/scheme
)

GENERATE_ENUM_SERIALIZATION(dictionary.h)
GENERATE_ENUM_SERIALIZATION(token.h)

END()

RECURSE_FOR_TESTS(ut)
