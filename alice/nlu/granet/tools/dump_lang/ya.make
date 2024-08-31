PROGRAM()

OWNER(samoylovboris)

SRCS(
    dump_granet_inflector.cpp
    dump_lemmer.cpp
    dump_normalizer.cpp
    dump_tokenizer.cpp
    main.cpp
)

PEERDIR(
    alice/nlu/granet/lib/lang
    alice/nlu/granet/lib/utils
    alice/nlu/libs/request_normalizer
    alice/nlu/libs/normalization
    alice/nlu/libs/tokenization
    alice/nlu/libs/ut_utils
    dict/dictutil
    kernel/inflectorlib/phrase
    kernel/lemmer/core
    kernel/lemmer/dictlib
    library/cpp/getopt
    library/cpp/langs
    library/cpp/token
    library/cpp/tokenizer
)

GENERATE_ENUM_SERIALIZATION(kernel/lemmer/dictlib/grammar_enum.h)
GENERATE_ENUM_SERIALIZATION(kernel/lemmer/core/lemmer.h)
GENERATE_ENUM_SERIALIZATION(library/cpp/langmask/langmask.h)
GENERATE_ENUM_SERIALIZATION(library/cpp/token/token_structure.h)

END()
