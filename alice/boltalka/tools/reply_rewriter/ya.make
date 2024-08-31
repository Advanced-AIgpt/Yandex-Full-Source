PY2_PROGRAM()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(
    __main__.py
    substitute_rewritten_replies.py
    substitute_replies.py
    sed_replacer.py
    base_replacer.py
    pipeline_replacer.py
    respect_replacer.py
    capitalizer.py
)

PEERDIR(
    alice/boltalka/py_libs/normalization
    alice/boltalka/py_libs/respect_classifier
    yt/python/client
    bindings/python/lemmer_lib
)

END()
