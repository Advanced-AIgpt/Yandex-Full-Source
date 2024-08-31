LIBRARY()

OWNER(
    alzaharov
    g:alice_boltalka
)

PEERDIR(
    alice/nlu/libs/tf_nn_model
    library/cpp/json
)

SRCS(
    emoji_classifier.cpp
    model.cpp
)

END()