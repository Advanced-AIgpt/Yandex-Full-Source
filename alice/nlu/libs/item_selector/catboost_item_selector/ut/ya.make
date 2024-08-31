UNITTEST_FOR(alice/nlu/libs/item_selector/catboost_item_selector)

OWNER(
    volobuev
    g:alice_quality
)

DATA(
    sbr://1244631599 # [model/model.cbm, model/spec.json]
)

FROM_SANDBOX(
    910062885
    OUT_NOAUTO
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
    library/cpp/testing/unittest
)

SRCS(
    catboost_item_selector_ut.cpp
)

END()
