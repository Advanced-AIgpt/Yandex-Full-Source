PY23_LIBRARY()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/nlu/py_libs/syntax_parser
    alice/nlu/py_libs/tf_model_applier
    alice/nlu/py_libs/tokenizer
    contrib/libs/tf/python
)

PY_SRCS(
    __init__.py
    respect_rewriter.py
    rewrite_classifiers.py
)

END()

RECURSE(
    models
)

RECURSE_FOR_TESTS(
    ut
)
