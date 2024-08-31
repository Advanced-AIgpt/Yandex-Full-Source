PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    a-square
    akhruslan
    stupnik
    vitvlkv
    g:hollywood
    g:alice
)

SRCS(
    cache_data.proto
    callback_payload.proto
    centaur.proto
    fast_data.proto
    music.proto
    music_arguments.proto
    music_context.proto
    music_hardcoded_arguments.proto
    music_memento_scenario_data.proto
    play_audio.proto
    request.proto
)

PEERDIR(
    alice/hollywood/library/phrases/proto
    alice/hollywood/library/tags/proto
    alice/megamind/protos/scenarios
)

EXCLUDE_TAGS(GO_PROTO)

END()
