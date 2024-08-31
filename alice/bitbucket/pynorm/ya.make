PY23_LIBRARY()

OWNER(akastornov)

PEERDIR(
    alice/bitbucket/pynorm/normalize/ctypes
)

PY_SRCS(
    TOP_LEVEL
    pynorm/__init__.py
    pynorm/fstnormalizer.py
)

END()
