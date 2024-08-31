PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    apphosted_tts_client_ut.py
    cache_ut.py
    cached_mds_ut.py
    common.py
    ttsutils_ut.py
    ttsstream_ut.py
)

PEERDIR(
    alice/uniproxy/library/backends_tts
    alice/uniproxy/library/testing

    alice/cuttlefish/library/protos
    alice/cuttlefish/library/python/apphost_message
    apphost/lib/grpc/protos
)

END()
