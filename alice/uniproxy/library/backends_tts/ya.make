PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    apphosted_tts_client.py
    audioplayer.py
    cache.py
    cached_mds.py
    opus_stream.py
    complex_opus_stream.py
    realtimestreamer.py
    ttsstream.py
    ttsutils.py
)

PEERDIR(
    alice/cachalot/client

    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/protos
    alice/uniproxy/library/settings
    alice/uniproxy/library/utils
    alice/uniproxy/library/global_state
    alice/uniproxy/library/backends_tts/background

    contrib/python/tornado/tornado-4
    contrib/python/cachetools/py3

    voicetech/library/proto_api
    voicetech/library/aproc/py
)

END()

RECURSE_FOR_TESTS(ut)
