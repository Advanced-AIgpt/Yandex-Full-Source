LIBRARY()

OWNER(
    g:hollywood
)

IF (SCENARIO_ALARM OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/alarm
    )
ENDIF()

IF (SCENARIO_ALICE_SHOW OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/alice_show
    )
ENDIF()

IF (SCENARIO_BLUEPRINTS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/blueprints
    )
ENDIF()

IF (SCENARIO_BLUETOOTH OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/bluetooth
    )
ENDIF()

IF (SCENARIO_BUGREPORT OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/bugreport
    )
ENDIF()

IF (SCENARIO_CEC_COMMANDS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/cec_commands
    )
ENDIF()

IF (SCENARIO_COUNT_ALOUD OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/count_aloud
    )
ENDIF()

IF (SCENARIO_DO_NOTHING OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/do_nothing
    )
ENDIF()

IF (SCENARIO_DRAW_PICTURE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/draw_picture
    )
ENDIF()

IF (SCENARIO_EQUALIZER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/equalizer
    )
ENDIF()

IF (SCENARIO_EXTERNAL_APP OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/smart_device/external_app
    )
ENDIF()

IF (SCENARIO_FAST_COMMAND OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/fast_command
    )
ENDIF()

IF (SCENARIO_FOOD OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/food
    )
ENDIF()

IF (SCENARIO_GENERAL_CONVERSATION OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/general_conversation
    )
ENDIF()

IF (SCENARIO_GET_DATE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/get_date
    )
ENDIF()

IF (SCENARIO_GET_TIME OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/get_time
    )
ENDIF()

IF (SCENARIO_GOODS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/goods
    )
ENDIF()

IF (SCENARIO_GOODWIN OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/goodwin
    )
ENDIF()

IF (SCENARIO_HAPPY_NEW_YEAR OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/happy_new_year
    )
ENDIF()

IF (SCENARIO_HARDCODED_RESPONSE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/hardcoded_response
    )
ENDIF()

IF (SCENARIO_HOW_MUCH OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/market/how_much
    )
ENDIF()

IF (SCENARIO_HOW_TO_SPELL OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/how_to_spell
    )
ENDIF()

IF (SCENARIO_IMAGE_WHAT_IS_THIS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/image_what_is_this
    )
ENDIF()

IF (SCENARIO_LINK_A_REMOTE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/link_a_remote
    )
ENDIF()

IF (SCENARIO_MAPS_DOWNLOAD_OFFLINE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/maps_download_offline
    )
ENDIF()

IF (SCENARIO_MARKET OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/market
    )
ENDIF()

IF (SCENARIO_MESSENGER_CALL OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/messenger_call
    )
ENDIF()

IF (SCENARIO_METRONOME OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/metronome
    )
ENDIF()

IF (SCENARIO_MORDOVIA_VIDEO_SELECTION OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/mordovia_video_selection
    )
ENDIF()

IF (SCENARIO_MUSIC OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/music
    )
ENDIF()

IF (SCENARIO_MUSIC_WHAT_IS_PLAYING OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/music_what_is_playing
    )
ENDIF()

IF (SCENARIO_NEWS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/news
    )
ENDIF()

IF (SCENARIO_NOTIFICATIONS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/notifications
    )
ENDIF()

IF (SCENARIO_ONBOARDING OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/onboarding
    )
ENDIF()

IF (SCENARIO_ONBOARDING_CRITICAL_UPDATE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/onboarding_critical_update
    )
ENDIF()

IF (SCENARIO_ORDER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/order
    )
ENDIF()

IF (SCENARIO_RANDOM_NUMBER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/random_number
    )
ENDIF()

IF (SCENARIO_REASK OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/reask
    )
ENDIF()

IF (SCENARIO_REMINDERS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/reminders
    )
ENDIF()

IF (SCENARIO_REPEAT OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/repeat
    )
ENDIF()

IF (SCENARIO_REPEAT_AFTER_ME OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/repeat_after_me
    )
ENDIF()

IF (SCENARIO_ROUTE_MANAGER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/route_manager
    )
ENDIF()

IF (SCENARIO_SEARCH OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/search
    )
ENDIF()

IF (SCENARIO_SETTINGS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/settings
    )
ENDIF()

IF (SCENARIO_SHOW_GIF OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/show_gif
    )
ENDIF()

IF (SCENARIO_SHOW_TRAFFIC_BASS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/show_traffic_bass
    )
ENDIF()

IF (SCENARIO_SSSSS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/sssss
    )
ENDIF()

IF (SCENARIO_SUBSCRIPTIONS_MANAGER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/subscriptions_manager
    )
ENDIF()

IF (SCENARIO_SUGGESTERS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/suggesters/games
        alice/hollywood/library/scenarios/suggesters/movies
    )
ENDIF()

IF (SCENARIO_TAXIMETER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/taximeter
    )
ENDIF()

IF (SCENARIO_TEST_SCENARIO OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/test_scenario
    )
ENDIF()

IF (SCENARIO_TIME_CAPSULE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/time_capsule
    )
ENDIF()

IF (SCENARIO_TR_NAVI OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/tr_navi/add_point
        alice/hollywood/library/scenarios/tr_navi/find_poi
        alice/hollywood/library/scenarios/tr_navi/general_conversation
        alice/hollywood/library/scenarios/tr_navi/get_my_location
        alice/hollywood/library/scenarios/tr_navi/get_weather
        alice/hollywood/library/scenarios/tr_navi/handcrafted
        alice/hollywood/library/scenarios/tr_navi/navi_external_confirmation
        alice/hollywood/library/scenarios/tr_navi/show_route
        alice/hollywood/library/scenarios/tr_navi/switch_layer
    )
ENDIF()

IF (SCENARIO_TRANSFORM_FACE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/transform_face
    )
ENDIF()

IF (SCENARIO_TV_CHANNELS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/tv_channels
    )
ENDIF()

IF (SCENARIO_TV_CHANNELS_EFIR OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/tv_channels_efir
    )
ENDIF()

IF (SCENARIO_TV_CONTROLS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/tv_controls
    )
ENDIF()

IF (SCENARIO_TV_HOME OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/tv_home
    )
ENDIF()

IF (SCENARIO_VIDEO OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/video
    )
ENDIF()

IF (SCENARIO_VIDEO_MUSICAL_CLIPS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/video_musical_clips
    )
ENDIF()

IF (SCENARIO_VIDEO_RATER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/video_rater
    )
ENDIF()

IF (SCENARIO_VIDEO_RECOMMENDATION OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/video_recommendation
    )
ENDIF()

IF (SCENARIO_VINS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/vins
    )
ENDIF()

IF (SCENARIO_VOICE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/voice
    )
ENDIF()

IF (SCENARIO_VOICEPRINT OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/voiceprint
    )
ENDIF()

IF (SCENARIO_WATCH_LIST OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/watch_list
    )
ENDIF()

IF (SCENARIO_WEATHER OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/weather
    )
ENDIF()

IF (SCENARIO_ZEN_SEARCH OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/zen_search
    )
ENDIF()

IF (SCENARIO_ZERO_TESTING OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        alice/hollywood/library/scenarios/zero_testing
    )
ENDIF()

END()
