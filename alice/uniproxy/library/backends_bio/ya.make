PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    yabiostream.py
    yabio_storage.py
)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/uniproxy/library/backends_common
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
    alice/uniproxy/library/utils

    contrib/python/tornado/tornado-4

    ydb/public/sdk/python

    voicetech/library/proto_api
)

END()

RECURSE_FOR_TESTS(ut)
