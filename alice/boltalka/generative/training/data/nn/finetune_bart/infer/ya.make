PY3_PROGRAM()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    # 3rd party
    ml/libs/ml_data_reader/src
    ml/tensorflow/tfnn/src
)

PY_SRCS(
    __init__.py
    __main__.py
)

END()
