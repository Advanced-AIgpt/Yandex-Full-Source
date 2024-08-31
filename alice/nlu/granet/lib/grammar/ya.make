LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    domain.cpp
    grammar.cpp
    grammar_data.cpp
    multi_grammar.cpp
    serialized_grammar.cpp
    token_id.cpp
)

PEERDIR(
    alice/nlu/granet/lib/grammar/proto
    alice/nlu/granet/lib/utils
    alice/nlu/libs/tuple_like_type
    library/cpp/containers/comptrie
    library/cpp/packers
)

GENERATE_ENUM_SERIALIZATION(grammar_data.h)
GENERATE_ENUM_SERIALIZATION(multi_grammar.h)

END()
