PY2_PROGRAM()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/nlu/py_libs/respect_rewriter
    contrib/libs/tf/python
    yt/python/client
)

PY_SRCS(
    MAIN main.py
)

END()

RECURSE(
    data
)
