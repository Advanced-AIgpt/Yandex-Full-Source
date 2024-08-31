PY3_LIBRARY()

OWNER(g:jupyter-cloud)

PY_SRCS(
    __init__.py
    role.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
