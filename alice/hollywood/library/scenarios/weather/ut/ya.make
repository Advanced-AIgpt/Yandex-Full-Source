UNITTEST_FOR(alice/hollywood/library/scenarios/weather)

OWNER(
    sparkle
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/frame
    alice/hollywood/library/request
    alice/hollywood/library/scenarios/weather
    alice/hollywood/library/scenarios/weather/background_sounds
    alice/hollywood/library/scenarios/weather/proto
    alice/library/client
    alice/library/datetime
    alice/library/logger
    alice/library/proto
    alice/library/util
    alice/megamind/protos/scenarios
    library/cpp/containers/stack_vector
    library/cpp/json/writer
    library/cpp/langs
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    alice/hollywood/library/scenarios/weather/util_ut.cpp
    alice/hollywood/library/scenarios/weather/handles/prepare_city_handle_ut.cpp
    alice/hollywood/library/scenarios/weather/util/avatars_ut.cpp
    alice/hollywood/library/scenarios/weather/util/translations_ut.cpp
)

END()
