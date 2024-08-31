PROGRAM()

OWNER(
    yorky0
    g:alice_quality
)

PEERDIR(
    alice/library/json
    alice/nlu/libs/entity_parsing
    alice/nlu/libs/entity_searcher
    library/cpp/getopt
    library/cpp/json
)

SRCS(
    build_entity_trie.cpp
)

END()
