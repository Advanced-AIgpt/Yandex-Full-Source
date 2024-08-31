PY3_LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    # 3rd party
    ml/tensorflow/tfnn/src
    ml/libs/ml_data_reader/src
)

PY_SRCS(
    __init__.py
    infer.py
    extensions/insertion_transformer/loss/__init__.py
    extensions/insertion_transformer/loss/xent.py
    extensions/insertion_transformer/__init__.py
    extensions/insertion_transformer/data.py
    extensions/insertion_transformer/decoder.py
    extensions/insertion_transformer/model.py
    extensions/insertion_transformer/simple_sliced_argmax.py
    extensions/insertion_transformer/util.py
    extensions/lm/__init__.py
    extensions/lm/model.py
)

END()
