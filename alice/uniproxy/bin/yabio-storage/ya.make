PY3_PROGRAM()
OWNER(g:voicetech-infra)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/tornado/tornado-4
    ydb/public/sdk/python

    alice/uniproxy/library/backends_bio
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging

    voicetech/library/proto_api
)

END()
