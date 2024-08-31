PY2_PROGRAM()

OWNER(
    sdll
    g:alice_quality
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/nlu/tools/lstm_classifier_trainer/core
    contrib/python/numpy
    yt/python/client
)

END()
