PROGRAM()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    batch_applet.cpp
    common_options.cpp
    dataset_applet.cpp
    debug_applet.cpp
    synonyms.cpp
    main.cpp
    normalizer.cpp
    grammar_applet.cpp
)

PEERDIR(
    alice/nlu/granet/lib
    alice/nlu/granet/tools/common
    alice/nlu/libs/request_normalizer
    alice/nlu/libs/lemmatization
    alice/nlu/libs/tokenization
    dict/dictutil
    dict/nerutil
    kernel/gazetteer
    library/cpp/getopt
    library/cpp/json
    library/cpp/langs
    search/wizard/common/thesaurus/proto
)

END()
