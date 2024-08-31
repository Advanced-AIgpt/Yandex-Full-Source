LIBRARY()

OWNER(
    samoylovboris
    vl-trifonov
    g:alice_quality
)

SRCS(
    tokenizer.cpp
)

GENERATE_ENUM_SERIALIZATION(tokenizer.h)

PEERDIR(
    alice/nlu/libs/interval
    alice/nlu/libs/normalization
    library/cpp/langs
    library/cpp/token
    library/cpp/tokenizer
    tools/enum_parser/enum_serialization_runtime
)

END()

RECURSE_FOR_TESTS(
    ut
)
