PY3_PROGRAM()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/protobuf

    ydb/public/sdk/python

    alice/uniproxy/library/backends_common
    alice/uniproxy/library/protos
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

END()
