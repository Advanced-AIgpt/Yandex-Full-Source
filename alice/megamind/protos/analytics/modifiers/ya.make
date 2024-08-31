PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

PEERDIR(
    alice/megamind/protos/analytics/modifiers/colored_speaker
    alice/megamind/protos/analytics/modifiers/conjugator
    alice/megamind/protos/analytics/modifiers/polyglot
    alice/megamind/protos/analytics/modifiers/proactivity
    alice/megamind/protos/analytics/modifiers/voice_doodle
    alice/megamind/protos/analytics/modifiers/whisper
    alice/megamind/protos/proactivity
    mapreduce/yt/interface/protos
)

SRCS(
    analytics_info.proto
)

END()

RECURSE(
    colored_speaker
    conjugator
    polyglot
    proactivity
    voice_doodle
    whisper
)
