PY2_LIBRARY()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/boltalka/py_libs/normalization
    bindings/python/lemmer_lib
    catboost/python-package/lib
    contrib/python/numpy
)

END()
