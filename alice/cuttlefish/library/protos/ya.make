PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:voicetech-infra
)

SRCS(
    antirobot.proto
    asr.proto
    audio.proto
    audio_separator.proto
    bio_context_save.proto
    bio_context_sync.proto
    context_load.proto
    context_save.proto
    events.proto
    megamind.proto
    music_match.proto
    personalization.proto
    session.proto
    store_audio.proto
    tts.proto
    uniproxy2.proto
    wsevent.proto
    yabio.proto
)

PEERDIR(
    alice/cachalot/api/protos
    alice/megamind/protos/common
    alice/megamind/protos/guest
    alice/megamind/protos/speechkit
    alice/library/censor/protos
    alice/protos/data
    apphost/lib/proto_answers
    voicetech/asr/engine/proto_api
    voicetech/library/proto_api
    voicetech/library/settings_manager/proto
)

CPP_PROTO_PLUGIN(
    prototraits alice/cuttlefish/tools/prototraits .traits.pb.h
)

EXCLUDE_TAGS(GO_PROTO)

GENERATE_ENUM_SERIALIZATION(personalization.pb.h)

END()
