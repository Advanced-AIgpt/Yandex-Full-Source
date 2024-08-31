OWNER(
    dan-anastasev
    g:hollywood
)

PY2_PROGRAM(suggestions_preparer)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/attrs
    contrib/python/requests
    ydb/public/sdk/python
)

END()
