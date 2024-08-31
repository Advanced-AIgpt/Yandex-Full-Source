UNITTEST_FOR(alice/nlu/libs/binary_classifier)

OWNER(
    dan-anastasev
    g:alice_quality
)

DEPENDS(
    alice/nlu/data/ru/boltalka_dssm
    search/wizard/data/wizard/AliceBinaryIntentClassifier
    search/wizard/data/wizard/AliceTokenEmbedder
)

SIZE(MEDIUM)

# Pairs of random texts and their expected embeddings
FROM_SANDBOX(
    FILE
    1238469874
    RENAME
    test_examples.json
    OUT_NOAUTO
    embedder_test_examples.json
)

# Pairs of texts and classes they should be certainty classified to
FROM_SANDBOX(
    FILE
    1243146517
    OUT_NOAUTO
    classifier_test_examples.json
)

# Pairs of texts and classes they should be certainty classified to as news
FROM_SANDBOX(
    FILE
    1821945903
    OUT_NOAUTO
    news_classifier_test_examples.json
)

FROM_SANDBOX(
    FILE
    2572040917
    RENAME model.cbm
    OUT_NOAUTO
    beggins_catboost_model.cbm
)

RESOURCE(
    embedder_test_examples.json          embedder_test_examples.json
    classifier_test_examples.json        classifier_test_examples.json
    news_classifier_test_examples.json   news_classifier_test_examples.json
    beggins_catboost_model.cbm           beggins_catboost_model.cbm
)

PEERDIR(
    library/cpp/resource
    library/cpp/testing/unittest
)

SRCS(
    binary_classifier_ut.cpp
)

END()
