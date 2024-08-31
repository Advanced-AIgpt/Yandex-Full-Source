UNITTEST_FOR(alice/bass)

OWNER(g:bass)

SIZE(MEDIUM)

DATA(arcadia/alice/bass/configs)

PEERDIR(
    alice/bass/libs/avatars
    alice/bass/libs/fetcher
    alice/bass/libs/globalctx
    alice/bass/libs/radio
    alice/bass/libs/source_request
    alice/bass/libs/ut_helpers
    alice/bass/libs/video_common
    alice/bass/libs/ydb_helpers
    alice/bass/ut/protos
    library/cpp/geobase
    library/cpp/http/coro
    library/cpp/http/server
    library/cpp/testing/unittest
    ydb/public/sdk/cpp/client/ydb_driver
)

ENV(YDB_YQL_SYNTAX_VERSION="0")

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

SRCS(
    forms/common/personal_data_ut.cpp
    forms/context/context_ut.cpp
    forms/directives_ut.cpp
    forms/external_skill/ifs_map_ut.cpp
    forms/external_skill/ut.cpp
    forms/navigator/user_bookmarks_ut.cpp
    forms/news/news_ut.cpp
    forms/player_command/player_command_ut.cpp
    forms/protocol_scenario/protocol_scenario_utils_ut.cpp
    forms/reminders/timer_ut.cpp
    forms/search/direct_continuation_ut.cpp
    forms/search/direct_gallery_ut.cpp
    forms/search/serp_ut.cpp
    forms/translate/translate_form_handler_ut.cpp
    forms/traffic_ut.cpp
    forms/urls_builder_ut.cpp
    forms/user_aware_handler_ut.cpp
    forms/video/availability_ut.cpp
    forms/video/billing_ut.cpp
    forms/video/change_track_ut.cpp
    forms/video/kinopoisk_provider_ut.cpp
    forms/video/kinopoisk_recommendations_ut.cpp
    forms/video/play_video_ut.cpp
    forms/video/protocol_scenario_helpers/intents_ut.cpp
    forms/video/protocol_scenario_helpers/intent_classifier_ut.cpp
    forms/video/protocol_scenario_helpers/request_creator_ut.cpp
    forms/video/protocol_scenario_helpers/utils_ut.cpp
    forms/video/show_video_settings_ut.cpp
    forms/video/skip_fragment_ut.cpp
    forms/video/utils_ut.cpp
    forms/video/vh_player_ut.cpp
    forms/video/video_how_long_ut.cpp
    forms/video/video_provider_ut.cpp
    forms/video/video_slots_ut.cpp
    forms/vins_ut.cpp
    forms/voiceprint/voiceprint_enroll_ut.cpp
    forms/voiceprint/voiceprint_remove_ut.cpp
    forms/weather/api_ut.cpp
    helpers.cpp
    http_request_ut.cpp
    libs/ner/ut/ner_ut.cpp
    libs/radio/recommender_ut.cpp
    libs/smallgeo/kdtree_ut.cpp
    libs/smallgeo/utils_ut.cpp
    test_users_ut.cpp
)

RESOURCE(
    alice/bass/data/translate_language_stems.txt lang_stems
)

RESOURCE(
    alice/bass/data/translate_languages.txt languages
)

RESOURCE(
    alice/bass/data/directives.json directives
)

FROM_SANDBOX(FILE 1007643623 OUT_NOAUTO weather_api_forecast.json)

RESOURCE(
    weather_api_forecast.json weather_api_forecast.json
)

DATA(sbr://3069941764=geodata6)

REQUIREMENTS(network:full)

END()
