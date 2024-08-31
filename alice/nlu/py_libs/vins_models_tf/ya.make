PY2MODULE(vins_models_tf)

OWNER(
    smirnovpavel
)

STRIP()

PYTHON2_ADDINCL()

PEERDIR(
    alice/nlu/libs/rnn_tagger
    alice/nlu/libs/nn_classifier
    alice/nlu/libs/tf_model_converter
    alice/nlu/libs/encoder
    util
)

BUILDWITH_CYTHON_CPP(
    vins_models_tf.pyx
    --module-name vins_models_tf
)

NO_WERROR()

END()
