LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music/music_request_builder
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/music/util
    alice/library/biometry
    alice/megamind/protos/common/required_messages
    alice/megamind/protos/scenarios
    alice/protos/data/language
)

JOIN_SRCS_GLOBAL(
    all.cpp
    ambient_sound.cpp
    bass.cpp
    callback.cpp
    fairy_tale.cpp
    impl.cpp
    main.cpp
    multiroom.cpp
    object.cpp
    onboarding.cpp
    player_command.cpp
    radio.cpp
    reask.cpp
    responses.cpp
    stream.cpp
    subgraph.cpp
)

END()
