PY23_LIBRARY()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/nlu/py_libs/tf_model_applier
    contrib/libs/tf/python
    contrib/python/attrs
    contrib/python/numpy
    contrib/python/pymorphy2
    contrib/python/six
    ml/text_processing/python-package/lib
    quality/neural_net/bert/bert/tokenization
    yt/python/client
)

PY_SRCS(
    __init__.py
    chu_liu_edmonds.py
    lemmatize_helper.py
    model.py
    parser.py
    pymorphy_tag_converter.py
    vectorizers.py
    vocabulary.py
)

END()

RECURSE(
    model
)

RECURSE_FOR_TESTS(
    ut
)
