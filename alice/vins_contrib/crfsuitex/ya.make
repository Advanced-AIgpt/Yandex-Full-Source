PY2_LIBRARY()

OWNER(
    akastornov
    g:alice
)

SRCS(
    src/crfsuitextagger.cpp
    src/crfsuitex_wrapper.cpp
    src/crfsuite.cpp
)

PY_SRCS(
    TOP_LEVEL
    crfsuitex.py
)

PEERDIR(
    contrib/libs/crfsuite
    contrib/restricted/boost/libs/python
)

PY_REGISTER(
    _crfsuitex
)

END()
