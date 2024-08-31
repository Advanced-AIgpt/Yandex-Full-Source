UNITTEST_FOR(alice/hollywood/library/modifiers/matchers)

OWNER(
    yagafarov
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/modifiers/internal/config/proto

    alice/library/proto

    library/cpp/testing/common
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
)

SRCS(
    exact_mapping_config_ut.cpp
)

DATA(
    arcadia/alice/hollywood/library/modifiers/modifiers/colored_speaker_modifier/config/key_groups.pb.txt
    arcadia/alice/hollywood/library/modifiers/modifiers/colored_speaker_modifier/config/mapping.pb.txt
    arcadia/alice/hollywood/library/modifiers/modifiers/voice_doodle_modifier/config/key_groups.pb.txt
    arcadia/alice/hollywood/library/modifiers/modifiers/voice_doodle_modifier/config/mapping.pb.txt
)

END()
