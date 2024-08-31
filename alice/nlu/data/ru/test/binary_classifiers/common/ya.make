PY3_LIBRARY()

PEERDIR(
    contrib/python/pytest
    contrib/python/requests

    yt/python/client
    yql/library/python
)

PY_SRCS(
    begemot.py
    conftest.py
)

END()
