PY2_LIBRARY()

OWNER(
    sdll
    g:alice_quality
)

PY_SRCS(
    data.py
    metrics.py
    model.py
)

PEERDIR(
    alice/nlu/py_libs/vins_models_tf/lib
    contrib/libs/tf/python
    contrib/python/attrs
    contrib/python/numpy
    contrib/python/scikit-learn
    yt/python/client
)

END()
