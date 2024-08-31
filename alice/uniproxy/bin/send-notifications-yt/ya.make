PY3_PROGRAM()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/click

    ydb/public/sdk/python

    alice/uniproxy/library/backends_common
    alice/uniproxy/library/protos
    alice/megamind/protos/common
)

END()
