OWNER(nerevar)

PY3_LIBRARY()

PEERDIR(
    contrib/python/num2words
)

PY_SRCS(
    aggregate.py
    normalize.py
    arabic_const.py
    tashaphyne_contrib.py
)

END()