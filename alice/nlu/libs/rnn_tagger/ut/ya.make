UNITTEST_FOR(alice/nlu/libs/rnn_tagger)

OWNER(
    dan-anastasev
    g:alice_quality
)

FROM_SANDBOX(
    910062885
    OUT
    model_description
    model.pb
)

FROM_SANDBOX(
    2777409146 # these are cut embeddings for this test only
    OUT_NOAUTO
    embeddings
    embeddings_dictionary.trie
)

RESOURCE(
    model_description          model_description
    model.pb                   model.pb
    embeddings                 embeddings
    embeddings_dictionary.trie embeddings_dictionary.trie
)

PEERDIR(
    alice/nlu/libs/embedder
    library/cpp/resource
    library/cpp/testing/unittest
)

SRCS(
    rnn_tagger_ut.cpp
)

END()
