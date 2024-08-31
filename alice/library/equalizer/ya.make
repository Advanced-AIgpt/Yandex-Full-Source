LIBRARY()

OWNER(
    g:alice
    sparkle
    zhigan
)

FROM_SANDBOX(FILE 2910935472 OUT_NOAUTO seed_to_preset_mapping.json)
FROM_SANDBOX(FILE 2911019643 OUT_NOAUTO presets.json)

RESOURCE(seed_to_preset_mapping.json seed_to_preset_mapping.json)
RESOURCE(presets.json presets.json)

PEERDIR(
    alice/hollywood/library/environment_state
    alice/megamind/protos/scenarios
    alice/protos/endpoint
)

SRCS(
    equalizer.cpp
)

END()

RECURSE_FOR_TESTS(ut)
