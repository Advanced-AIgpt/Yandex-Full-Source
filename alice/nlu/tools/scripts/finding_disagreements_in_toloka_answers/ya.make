PY3_PROGRAM()

OWNER(
    g:alice_quality
    sha43
)

PEERDIR(
    contrib/python/pandas/py3
    contrib/python/numpy/py3
)

PY_SRCS(
    MAIN finding_disagreements_in_toloka_answers.py
)

END()
