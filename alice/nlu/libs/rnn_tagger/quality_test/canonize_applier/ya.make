PROGRAM()

OWNER(
    dan-anastasev
    igor-darov
    g:alice_quality
)

PEERDIR(
    alice/library/json
    alice/nlu/libs/embedder
    alice/nlu/libs/occurrence_searcher
    alice/nlu/libs/rnn_tagger
    library/cpp/getopt/small
    library/cpp/json
    search/begemot/rules/occurrences/custom_entities/rule/proto
)

SRCS(
    main.cpp
)

END()
