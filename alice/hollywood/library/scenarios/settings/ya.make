LIBRARY()

OWNER(
    lavv17
    g:alice_quality
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/settings/common
    alice/hollywood/library/scenarios/settings/nlg
    alice/library/proto
)

SRCS(
    GLOBAL settings.cpp
    music_announce.cpp
    devices_settings.cpp
)

GENERATE_ENUM_SERIALIZATION(devices_settings.h)

END()

RECURSE(
    common
)

RECURSE_FOR_TESTS(it2)
