LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    debug.cpp
    element_occurrence.cpp
    multi_parser.cpp
    parser.cpp
    preprocessed_sample.cpp
    result.cpp
    result_builder.cpp
    state.cpp
    state_dumper.cpp
)

PEERDIR(
    alice/nlu/granet/lib/grammar
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/utils
    library/cpp/containers/comptrie
    library/cpp/packers
    library/cpp/scheme
    library/cpp/tokenizer
)

GENERATE_ENUM_SERIALIZATION(state.h)

END()
