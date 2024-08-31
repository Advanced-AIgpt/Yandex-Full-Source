PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/libs/grpc/python
    contrib/python/tornado/tornado-4
    contrib/python/google-cloud-speech

    voicetech/library/proto_api
    alice/rtlog/client/python/lib
    alice/uniproxy/library/activation_storage
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
    alice/uniproxy/library/utils
)

PY_SRCS(
    __init__.py
    asr_json_adapter.py
    google.py
    spotterstream.py
    yaldistream.py
)

END()

RECURSE_FOR_TESTS(ut)
