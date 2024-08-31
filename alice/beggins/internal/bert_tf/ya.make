LIBRARY()

OWNER(dan4fedor)

SRCS(
    applier.cpp
    preparer.cpp
    tokenizer.cpp
    literals.cpp
    trie.cpp
)

PEERDIR(
    dict/libs/segmenter
    dict/libs/trie
    library/cpp/tf/graph_processor_base
)

END()

RECURSE_FOR_TESTS(
    ut
    benchmark
)
