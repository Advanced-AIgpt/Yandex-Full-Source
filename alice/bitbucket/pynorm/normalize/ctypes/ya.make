OWNER(akastornov)

PY23_LIBRARY()

PEERDIR(
    alice/bitbucket/pynorm/normalize
    library/python/ctypes
)

SRCS(
    syms.c
)

PY_REGISTER(
    alice.bitbucket.pynorm.normalize.ctypes.syms
)

PY_SRCS(
    __init__.py
)

END()
