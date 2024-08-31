LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/hollywood/library/request
    alice/library/logger
    alice/megamind/protos/scenarios
)

SRCS(
    last_play_timestamp.cpp
    player_features.cpp
)

GENERATE_ENUM_SERIALIZATION(player_features.h)

END()

RECURSE_FOR_TESTS(ut)
