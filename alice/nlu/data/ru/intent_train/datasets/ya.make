RECURSE(
    pool
    target
)

PY3_PROGRAM(filter_by_intent)

OWNER(kudrinsky)

PEERDIR(
    contrib/libs/tf/python
    contrib/python/numpy
    contrib/python/click
    contrib/python/pandas
)

PY_SRCS(
    MAIN filter.py
)

END()
