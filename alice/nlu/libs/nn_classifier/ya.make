LIBRARY(nn_classifier)

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
    nn_classifier.cpp
)

NO_WERROR()

END()
