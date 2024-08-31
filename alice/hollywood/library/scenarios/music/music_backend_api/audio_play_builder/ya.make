LIBRARY()

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

JOIN_SRCS(
    all.cpp
    audio_play_builder.cpp
    callback_payload_builder.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/music_backend_api/play_audio
    alice/hollywood/library/scenarios/music/proto
    alice/megamind/protos/scenarios
)

END()
