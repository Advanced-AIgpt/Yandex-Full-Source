PY23_LIBRARY()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    contrib/python/attrs
    ml/text_processing/python-package/lib
)

PY_SRCS(
    __init__.py
    tokenizer.py
)

END()

RECURSE_FOR_TESTS(
    ut/py2
    ut/py3
)
