PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    yabiostream_ut.py
    yabio_storage_ut.py
    mocks.py
)

PEERDIR(
    alice/uniproxy/library/backends_common/mocks
    alice/uniproxy/library/backends_bio
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
    alice/uniproxy/library/testing

    contrib/python/tornado/tornado-4

    voicetech/library/proto_api
)

END()
