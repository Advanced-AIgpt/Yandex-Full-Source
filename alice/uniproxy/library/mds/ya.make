PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PEERDIR(
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/auth
    contrib/python/botocore
    library/python/awssdk_async_extensions/lib/core
)

PY_SRCS(
    __init__.py
    mds.py
    s3.py
)

END()
