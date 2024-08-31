PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    apikey.py
    apphost.py
    async_grpc_call.py
    context_save.py
    httpstream.py
    protohelpers.py
    protostream.py
    storage.py
    vault_client.py
    ydb.py
)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/auth
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
    alice/cuttlefish/library/python/apphost_message
    alice/cuttlefish/library/protos
    alice/megamind/protos/speechkit

    contrib/python/protobuf
    contrib/python/tornado/tornado-4

    ydb/public/sdk/python
    kikimr/public/sdk/python/tvm
)

END()

RECURSE_FOR_TESTS(
    ut
)
