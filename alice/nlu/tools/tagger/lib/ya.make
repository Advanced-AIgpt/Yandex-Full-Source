PY2_LIBRARY()

OWNER(
    the0
    g:alice_quality
)

PEERDIR(
    alice/nlu/py_libs/tagger
    alice/nlu/py_libs/utils
    contrib/python/attrs
    contrib/python/numpy
    yt/python/client
)

PY_SRCS(
    __init__.py
    data.py
    sample_features.py
)

END()
