PY3_LIBRARY()

OWNER(
    g:hollywood
    sparkle
)

PEERDIR(
    alice/tests/library/service
    library/python/codecs
)

PY_SRCS(
    __init__.py
    conftest.py
    graph.py
    marks.py
    mock_server.py
    wrappers.py
)

END()
