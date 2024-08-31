PY2_LIBRARY()

OWNER(
    akastornov
    smirnovpavel
)

PEERDIR(
    alice/nlu/libs/rnn_tagger
    alice/nlu/libs/nn_classifier
    alice/nlu/libs/tf_model_converter
    alice/nlu/libs/encoder
    util
)

PY_SRCS(
    alice/nlu/py_libs/vins_models_tf/vins_models_tf.pyx=vins_models_tf
)

NO_WERROR()

END()
