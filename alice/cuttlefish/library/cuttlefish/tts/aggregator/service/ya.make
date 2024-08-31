LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/tts/aggregator/audio_source
    alice/cuttlefish/library/cuttlefish/tts/aggregator/output_audio_stream

    alice/cuttlefish/library/cuttlefish/stream_servant_base

    alice/cuttlefish/library/proto_censor
)

END()
