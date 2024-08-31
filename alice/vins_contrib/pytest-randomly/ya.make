PY23_LIBRARY()

OWNER(
    akastornov
    g:alice
)

VERSION(1.2.3)

LICENSE(BSD-3-Clause)

PEERDIR(
    contrib/python/pytest
)

PY_SRCS(
    TOP_LEVEL
    pytest_randomly.py
)

PY_SRCS(
    conftest.py
)

END()
