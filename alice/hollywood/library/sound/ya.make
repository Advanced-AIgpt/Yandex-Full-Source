LIBRARY()

OWNER(
    hellodima
    olegator
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/response
    alice/library/json
    alice/megamind/protos/common
)

SRCS(
    device_volume.cpp
    sound_change.cpp
    sound_common.cpp
    sound_level_calculation.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
