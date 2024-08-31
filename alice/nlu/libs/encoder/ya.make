LIBRARY(encoder)

OWNER(
    smirnovpavel
)

PEERDIR(
    alice/nlu/libs/tf_nn_model
    library/cpp/tf/graph_processor_base
    contrib/libs/tf
    util
)

SRCS(
    encoder.cpp
    word_nn.cpp
)

NO_WERROR()

END()
