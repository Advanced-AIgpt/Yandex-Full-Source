PY2_PROGRAM()

OWNER(
    the0
    g:alice_quality
)

PEERDIR(
    alice/nlu/py_libs/tagger
    alice/nlu/tools/tagger/lib
    contrib/python/numpy
    yt/python/client
)

PY_SRCS(
    MAIN main.py
)

END()
