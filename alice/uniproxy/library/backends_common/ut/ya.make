PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    common.py
    apphost_ut.py
    apikey_ut.py
    protostream_ut.py
    storage_ut.py
    context_save_ut.py
)

PEERDIR(
    contrib/python/tornado/tornado-4
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/settings
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/python/apphost_message
    alice/megamind/protos/speechkit
    apphost/lib/grpc/protos
)

END()
