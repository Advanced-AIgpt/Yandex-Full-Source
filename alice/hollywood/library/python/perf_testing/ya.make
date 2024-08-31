PY2_LIBRARY()

OWNER(
    vitvlkv
    g:hollywood
)

PY_SRCS(
    __init__.py
    nanny_finder.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
