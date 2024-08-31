LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    compiler.cpp
    compiler_data.cpp
    data_loader.cpp
    directives.cpp
    element_modification.cpp
    element_writer.cpp
    expression_tree.cpp
    expression_tree_builder.cpp
    messages.cpp
    messages_en.cpp
    messages_ru.cpp
    nlu_line.cpp
    optimization_info_builder.cpp
    preprocessor.cpp
    rule_parser.cpp
    skip_params_calculator.cpp
    source_text_collection.cpp
    src_line.cpp
    sub_expression_element.cpp
    syntax.cpp
)

PEERDIR(
    alice/library/compression
    alice/nlu/granet/lib/grammar
    alice/nlu/granet/lib/lang
    alice/nlu/granet/lib/parser
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/utils
    alice/nlu/libs/interval
    alice/nlu/libs/lemmatization
    alice/nlu/libs/tokenization
    alice/nlu/libs/tuple_like_type
    dict/nerutil
    library/cpp/dbg_output
    library/cpp/containers/comptrie
    library/cpp/iterator
    library/cpp/packers
    library/cpp/regex/pcre
)

GENERATE_ENUM_SERIALIZATION(directives.h)
GENERATE_ENUM_SERIALIZATION(expression_tree.h)
GENERATE_ENUM_SERIALIZATION(messages.h)

END()

RECURSE_FOR_TESTS(ut)
