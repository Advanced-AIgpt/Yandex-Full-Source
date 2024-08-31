PY23_LIBRARY()

OWNER(g:megamind)

PY_SRCS(
    __init__.py
    run_info.py
    util.py
)

PEERDIR(
    contrib/python/requests
    library/python/testing/yatest_common
)

END()
