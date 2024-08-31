PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    settings_ut.py
    spotter_maps_ut.py
    topic_maps_ut.py
)

PEERDIR(
    library/python/resource
    alice/uniproxy/library/settings
)

END()
