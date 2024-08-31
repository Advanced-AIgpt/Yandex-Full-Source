LIBRARY()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/begemot/lib/fresh_options
    alice/boltalka/libs/text_utils
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/utils
    alice/nlu/libs/binary_classifier/proto
    alice/nlu/libs/embedder
    alice/nlu/libs/encoder
    alice/nlu/libs/tf_nn_model
    catboost/libs/model
    contrib/libs/protobuf
    kernel/dssm_applier/nn_applier/lib
    search/begemot/core
    library/cpp/dot_product
    library/cpp/iterator
    library/cpp/json
    library/cpp/langs
)

SRCS(
    beggins_binary_classifier.cpp
    binary_classifier_collection.cpp
    boltalka_dssm_embedder.cpp
    dssm_based_binary_classifier.cpp
    lstm_based_binary_classifier.cpp
    mixed_classifier.cpp
    mixed_input.cpp
    model_description.cpp
)

GENERATE_ENUM_SERIALIZATION(binary_classifier_collection.h)

END()

RECURSE(
    proto
    train
)

RECURSE_FOR_TESTS(
    ut
)
