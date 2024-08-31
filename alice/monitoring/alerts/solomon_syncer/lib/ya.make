PY3_LIBRARY()

OWNER(g:alice_fun)

PEERDIR(
    contrib/python/requests
)

PY_SRCS(
    sync.py
)

END()

RECURSE_FOR_TESTS(ut)
