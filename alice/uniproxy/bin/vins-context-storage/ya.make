PY3_PROGRAM()
OWNER(g:voicetech-infra)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/tornado/tornado-4
    ydb/public/sdk/python
    alice/uniproxy/library/vins_context_storage
    alice/uniproxy/library/logging
    alice/uniproxy/library/backends_common
)

END()
