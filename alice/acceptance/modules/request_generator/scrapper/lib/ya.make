PY3_LIBRARY()

OWNER(
    g:alice_downloaders
)

PEERDIR(
    contrib/python/attrs
    library/python/cyson
    alice/acceptance/modules/request_generator/lib
)

PY_SRCS(
    api/request.py
    api/response.py
    api/run.py
    io.py
    session.py
)

END()
