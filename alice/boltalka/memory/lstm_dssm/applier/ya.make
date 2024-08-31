LIBRARY()

OWNER(
    alzaharov
    g:alice_boltalka
)

SRCS(
    dssm_applier.cpp
    lstm_applier.cpp
)

PEERDIR(
    alice/nlu/libs/tf_nn_model
    library/cpp/json
)

END()