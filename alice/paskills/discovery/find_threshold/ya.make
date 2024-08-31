OWNER(g:paskills)

PY3_PROGRAM()

PY_SRCS(
    MAIN find_threshold.py
)

PEERDIR(
    contrib/python/six
    contrib/python/matplotlib
    contrib/python/scikit-learn
)

END()
