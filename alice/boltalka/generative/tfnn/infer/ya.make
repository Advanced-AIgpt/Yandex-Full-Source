PY3_PROGRAM()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/generative/tfnn/infer_lib
    alice/boltalka/generative/tfnn/preprocess
    alice/boltalka/generative/training/data/tokenizer_py
)

PY_SRCS(
    TOP_LEVEL
    __init__.py
    infer_model.py
)

PY_MAIN(
    infer_model
)

END()
