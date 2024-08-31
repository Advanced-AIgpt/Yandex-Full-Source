PY2_LIBRARY()

OWNER(
    the0
    g:alice_quality
)

PEERDIR(
    contrib/libs/tf/python
    contrib/python/attrs
    contrib/python/numpy
    contrib/python/scikit-learn
    alice/nlu/py_libs/vins_models_tf/lib
)

PY_SRCS(
    __init__.py
    tagger.py
)

END()
