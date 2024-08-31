LIBRARY(tf_nn_model)

OWNER(
    smirnovpavel
)

PEERDIR(
    library/cpp/tf/graph_processor_base
    util
)

SRCS(
    tf_nn_model.cpp
    batch_helpers.cpp
)

NO_WERROR()

END()
