PY2_LIBRARY()

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    alice/nlu/verstehen/util
    # 3rd party
    bindings/python/lemmer_lib
)

PY_SRCS(
    NAMESPACE verstehen.preprocess
    __init__.py
    filtering.py
    preprocessing.py
)

END()
