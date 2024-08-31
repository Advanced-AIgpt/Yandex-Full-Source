UNITTEST_FOR(alice/nlu/libs/anaphora_resolver/matchers/lstm)

OWNER(
    smirnovpavel
    g:alice_quality
)

RESOURCE(
    embeddings                 embeddings
    embeddings_dictionary.trie embeddings_dictionary.trie
)

FROM_SANDBOX(
    2777409146 # these are cut embeddings for this test only
    OUT_NOAUTO
    embeddings
    embeddings_dictionary.trie
)

DEPENDS(
    search/wizard/data/wizard/AliceAnaphoraMatcher
)

PEERDIR(
    alice/begemot/lib/session_conversion
    library/cpp/resource
    library/cpp/testing/unittest
)

SRCS(
    lstm_ut.cpp
)

END()
