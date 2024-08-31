UNITTEST_FOR(alice/megamind/library/scenarios/interface)

OWNER(g:megamind)

PEERDIR(
    alice/library/frame
    alice/megamind/protos/nlg
    alice/megamind/library/testing
    library/cpp/testing/unittest
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    blackbox_ut.cpp
    data_sources_ut.cpp
    quasar_devices_info_ut.cpp
    scenario_env_ut.cpp
)

RESOURCE(
    resources/alice4business_config.json alice4business_config.json
    resources/auxiliary_config.json auxiliary_config.json
    resources/begemot_fixlist.json begemot_fixlist.json
    resources/notification_state.json notification_state.json
    resources/wizard_response.json wizard_response.json
    resources/vins_rules.json vins_rules.json
    resources/video_view_state.json video_view_state.json
    resources/video_currently_playing.json video_currently_playing.json
)

END()
