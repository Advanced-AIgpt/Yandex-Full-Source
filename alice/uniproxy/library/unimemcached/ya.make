PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    client.py
    connection.py
    const.py
    pool.py
    client_mock.py
    yp_client.py
)

PEERDIR(
    alice/uniproxy/library/logging
    contrib/python/pyketama
    contrib/python/tornado/tornado-4
)

END()

RECURSE_FOR_TESTS(ut)
