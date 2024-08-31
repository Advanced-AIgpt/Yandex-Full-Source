LIBRARY()

OWNER(
    alexanderplat
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/modifiers/base_modifier
    alice/hollywood/library/modifiers/modifiers/polyglot_modifier/proto
    alice/hollywood/library/modifiers/registry
    alice/library/apphost_request
    alice/library/experiments
    alice/library/json
    alice/nlg/library/voice_prefix
    apphost/lib/proto_answers
    library/cpp/langs
    library/cpp/protobuf/util
)

SRCS(
    output_speech_modifier.cpp
    polyglot_modifier.cpp
    GLOBAL register.cpp
)

END()

RECURSE(
    proto
    ut
)
