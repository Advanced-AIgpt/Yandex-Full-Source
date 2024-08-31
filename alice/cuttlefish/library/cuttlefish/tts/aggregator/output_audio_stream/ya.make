LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    base.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/tts/aggregator/audio_source

    voicetech/library/aproc
    voicetech/library/mime
    voicetech/library/ogg
)

END()
