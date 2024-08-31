OWNER(g:hollywood)

PY3_PROGRAM(hollywood_recipe)

PY_SRCS(__main__.py)

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
    contrib/python/psutil
)

END()
