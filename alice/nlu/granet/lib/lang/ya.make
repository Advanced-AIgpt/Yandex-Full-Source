LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    inflector.cpp
    simple_tokenizer.cpp
    string_with_weight.cpp
    synonym_generator.cpp
    word_logprob_table.cpp
)

PEERDIR(
    alice/nlu/granet/lib/utils
    alice/nlu/libs/lemmatization
    alice/nlu/libs/normalization
    alice/nlu/libs/tuple_like_type
    dict/nerutil
    kernel/inflectorlib/phrase
    kernel/lemmer
    library/cpp/string_utils/indent_text
    library/cpp/token
    library/cpp/tokenizer
)

RESOURCE(
    data/word_logprob_ru.txt /granet/lang/word_logprob_ru.txt
)

END()

RECURSE_FOR_TESTS(ut)
