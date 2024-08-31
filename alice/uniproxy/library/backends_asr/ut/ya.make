PY3TEST()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/backends_asr
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/testing
    alice/uniproxy/library/utils
    voicetech/library/proto_api
    voicetech/asr/engine/proto_api
)

TEST_SRCS(
    common.py
    test_spotterstream.py
    test_yaldistream.py
    test_yaldistream_mocked.py
)

END()
