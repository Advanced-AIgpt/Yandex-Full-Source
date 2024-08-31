UNITTEST_FOR(alice/nlu/libs/ellipsis_rewriter)

OWNER(
    dan-anastasev
    g:alice_quality
)

DEPENDS(
    search/wizard/data/wizard/AliceEllipsisRewriter
)

FROM_SANDBOX(
    1110993285
    OUT_NOAUTO
    embeddings
    embeddings_dictionary.trie
)

RESOURCE(
    embeddings                 embeddings
    embeddings_dictionary.trie embeddings_dictionary.trie
)

PEERDIR(
    library/cpp/resource
    library/cpp/testing/unittest
)

SRCS(
    ellipsis_rewriter_ut.cpp
)

END()
