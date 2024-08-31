PY3TEST()

OWNER(mihajlova)

SIZE(LARGE)

FORK_TESTS()
SPLIT_FACTOR(30)

TEST_SRCS(
    alice_wrapper.py
    conftest.py
    opus.py

    actions/testpalm_actions.py

    automotive/testpalm_add_point.py
    automotive/testpalm_command.py
    automotive/testpalm_greeting.py
    automotive/testpalm_layers.py
    automotive/testpalm_speaker_voice.py

    advisers/common.py
    advisers/testpalm_game_suggest.py
    advisers/testpalm_movie_suggest.py

    alice_show/alice_show.py

    biometry/test_enroll.py
    biometry/testpalm_biometry.py
    biometry/testpalm_biometry_name.py

    bluetooth/player.py
    bluetooth/testpalm_bluetooth.py

    bugreport/bugreport.py

    call/contacts.py
    call/messenger_call.py
    call/testpalm_emergency.py
    call/testpalm_phone.py

    cec_commands/screen_on_off.py

    combinators/centaur_combinator/test_teasers.py
    combinators/centaur_main_screen/test_main_screen.py

    computer_vision/div_card.py
    computer_vision/image_what_is_this_activate_mm.py
    computer_vision/image_what_is_this_stub_answers.py
    computer_vision/image_what_is_this_entity.py
    computer_vision/image_what_is_this_clothes.py
    computer_vision/image_what_is_this_market.py
    computer_vision/image_what_is_this_ocr.py
    computer_vision/image_what_is_this_similar_artwork.py
    computer_vision/image_what_is_this_similar_people.py
    computer_vision/image_what_is_this_tags.py
    computer_vision/util.py
    computer_vision/testpalm_computer_vision.py

    context_saving/testpalm_context_saving.py

    conversation/test_general_conversation.py
    conversation/test_general_conversation_experiment.py
    conversation/test_movie_akinator.py
    conversation/test_generative_tale.py
    conversation/test_generative_toast.py
    conversation/testpalm_general_conversation.py

    disk/testpalm_disk.py

    do_nothing/do_nothing.py

    exchange_rate/testpalm_exchange_rate.py

    external_app/lg_open_external_app.py

    external_skills/common.py
    external_skills/game_of_cities.py
    external_skills/games.py
    external_skills/skill_integration.py
    external_skills/testpalm_external_skills.py
    external_skills/thin_player.py
    external_skills/universal_skill.py
    external_skills/skill_discovery.py

    flash_briefings/activate_radionews.py

    facts_and_objects/elari_promo.py
    facts_and_objects/push_notification.py
    facts_and_objects/ellipsis_buttons.py
    facts_and_objects/search_facts.py
    facts_and_objects/search_related_facts.py
    facts_and_objects/testpalm_dialogs.py
    facts_and_objects/testpalm_dialogs_experiment.py
    facts_and_objects/testpalm_search.py

    fm_radio/auto_commands.py
    fm_radio/stations.py

    food/food.py

    goodwin/covid.py

    hardcoded_responses/testpalm_hardcoded_responses.py

    iot/color_lamp.py
    iot/common.py
    iot/curtain.py
    iot/delayed_actions.py
    iot/group.py
    iot/iot_corner_cases.py
    iot/iot_demo.py
    iot/iot_promo.py
    iot/lamp_scene.py
    iot/lamp_with_config.py
    iot/nlg.py
    iot/scenario.py
    iot/room.py
    iot/testpalm_iot.py
    iot/testpalm_iot_sockets.py
    iot/vacuum_cleaner.py
    iot/voice_queries.py
    iot/white_lamp.py

    link_a_remote/testpalm_link_a_remote.py

    maps/download_offline.py

    market/analytics_info.py
    market/choice.py
    market/div_card.py
    market/how_much.py
    market/orders_status.py
    market/recurring_purchase.py
    market/shopping_list.py
    market/testpalm_how_much.py
    market/testpalm_market.py
    market/testpalm_market_beru.py
    market/testpalm_orders_status.py
    market/testpalm_recurring_purchase.py
    market/testpalm_shopping_list.py
    market/util.py

    megamind/test_active_actions.py
    megamind/test_arabic.py
    megamind/test_nlg_render_history.py
    megamind/test_speechkit.py

    metronome/test_metronome.py

    microintents/microintents.py

    ml/draw_picture.py
    ml/transform_face.py

    mordovia/mordovia.py
    mordovia/mordovia_as_homepage.py
    mordovia/mordovia_spa.py
    mordovia/mordovia_video_selection.py
    mordovia/mordovia_webview_navigation.py
    mordovia/testpalm_mordovia.py

    music/anaphora.py
    music/change_track_version.py
    music/discovery.py
    music/div_card.py
    music/fairy_tale.py
    music/fixlist.py
    music/generative.py
    music/internal_music_player.py
    music/lg_play_music.py
    music/meditation.py
    music/multiroom.py
    music/multiroom_iot_config.py
    music/multiroom_quality.py
    music/music_play_less.py
    music/music_reask.py
    music/muzpult.py
    music/onboarding.py
    music/personal_playlists.py
    music/player_commands.py
    music/player_commands_thin.py
    music/podcast_child.py
    music/redirect_to_thin_client.py
    music/search.py
    music/search_thin_client.py
    music/sounds.py
    music/station_promo.py
    music/stop_music.py
    music/streams.py
    music/testpalm_music.py
    music/testpalm_music_thin_client.py
    music/testpalm_music_02.py
    music/testpalm_sing_song.py
    music/thin_client.py
    music/thin_fm_radio.py
    music/vins_tests.py
    music/what_is_playing.py
    music/yamusic_audiobranding.py

    news/news_test_cases.py
    news/news_regexp.py

    notifications/notifications.py

    onboarding/lg_onboarding.py
    onboarding/test_configure_success.py
    onboarding/test_greeting.py
    onboarding/test_what_can_you_do.py
    onboarding/testpalm_onboarding.py

    online_cinema/test_online_cinema.py

    open_sites_and_apps/apps_fixlist.py
    open_sites_and_apps/open_turboapps.py
    open_sites_and_apps/testpalm_open_sites_and_apps.py

    pc_win/testpalm_pc_win.py

    pogoda/common.py
    pogoda/get_weather.py
    pogoda/get_weather_nowcast.py
    pogoda/get_weather_nowcast_prec_map.py
    pogoda/get_weather_pressure.py
    pogoda/get_weather_wind.py
    pogoda/pogoda.py
    pogoda/pogoda_led.py
    pogoda/testpalm_pogoda_dialogs.py
    pogoda/testpalm_pogoda_nowcast.py

    poi/div_card.py
    poi/gallery_poi.py
    poi/open_uri.py
    poi/testpalm_poi.py

    proactivity/test_postroll.py

    quality_storage/test_quality_storage.py

    radio/radio_onboarding.py
    radio/radio_play.py

    random_number/testpalm_random.py

    reask/reask.py

    reminders_and_todos/reminders.py
    reminders_and_todos/testpalm.py

    recipes/test_recipes.py

    repeat/testpalm_repeat.py

    route/__init__.py
    route/build_route.py
    route/div_card.py
    route/favorite.py
    route/get_to_place.py
    route/left_to_goal.py
    route/next_scenario.py
    route/set_addresses.py
    route/show_route_on_map.py
    route/testpalm_route.py
    route/via_points.py

    route_manager/route_manager.py

    search_scenario/images_gallery.py
    search_scenario/search_suggests.py
    search_scenario/testpalm.py
    search_scenario/test_centaur.py
    search_scenario/test_recipe_preroll.py

    settings/devices_settings.py

    show_gif/test_show_gif.py

    show_promo/test_shazam.py

    side_speech/side_speech.py

    sleep_timers/testpalm.py

    station_lite/test_hardcoded_playlists.py
    station_lite/test_modifiers.py

    station_scenarios/testpalm_go_back.py
    station_scenarios/testpalm_navigation_commands.py

    stop_command/testpalm_hollywood_stop.py
    stop_command/vins_stop.py

    taximeter/requestconfirm_tests.py

    taxi/testpalm_taxi.py

    thereminvox/thereminvox.py

    time_and_day/common.py
    time_and_day/testpalm_day.py
    time_and_day/testpalm_old_auto.py
    time_and_day/testpalm_time_ar.py
    time_and_day/testpalm_time.py

    timers_and_alarms/test_alarm_directives.py
    timers_and_alarms/test_alarm_directives_ar.py
    timers_and_alarms/test_morning_show_alarm.py
    timers_and_alarms/test_music_alarm.py
    timers_and_alarms/test_timer_directives.py
    timers_and_alarms/testpalm_alarms.py
    timers_and_alarms/testpalm_alarms_ar.py
    timers_and_alarms/testpalm_timers.py

    tr_navi/add_point/add_point.py
    tr_navi/find_poi/find_poi.py
    tr_navi/general_conversation/general_conversation.py
    tr_navi/get_my_location/get_my_location.py
    tr_navi/get_weather/testpalm_get_weather.py
    tr_navi/handcrafted/handcrafted.py
    tr_navi/show_route/testpalm_show_route.py
    tr_navi/switch_layer/switch_layer.py

    translate/testpalm_1055.py
    translate/testpalm_1057.py
    translate/testpalm_1060.py
    translate/testpalm_1459.py
    translate/testpalm_1876.py

    trash_test/trash_test.py

    traffic/testpalm_traffic.py
    traffic/test_irrelevant_traffic.py

    tv/div_card.py
    tv/testpalm_tv_searchapp.py
    tv/testpalm_tv_station.py
    tv/testpalm_tv_yabro.py
    tv/tv_channels.py
    tv/subscription_channels.py

    video/change_form.py
    video/lg_play_video.py
    video/open_current_video.py
    video/pay_push.py
    video/play_next_video.py
    video/play_video_with_tracks.py
    video/play_vs_description.py
    video/search_video.py
    video/show_view_video.py
    video/stop_video.py
    video/testpalm_video.py
    video/open_video_new.py
    video/select_item.py

    video_rater/testpalm_video_rater.py

    video_command/change_track.py
    video_command/recovery_tracks.py
    video_command/show_video_settings.py
    video_command/skip_video_fragment.py
    video_command/video_how_long.py

    video_trailer/video_trailer.py

    vins/test_forbidden_intents.py
    vins/test_invalid_force_intent.py
    vins/test_response.py

    voice/nlg.py
    voice/whisper.py

    volume/testpalm_hollywood_volume.py
    volume/testpalm_volume.py

    where_am_i/testpalm.py

    zero_testing/test_zero_testing.py
)

PEERDIR(
    alice/tests/integration_tests/notifications/notificator_client
    alice/tests/integration_tests/notifications/subscriptions
    alice/tests/library/auth
    alice/tests/library/directives
    alice/tests/library/intent
    alice/tests/library/locale
    alice/tests/library/mark
    alice/tests/library/region
    alice/tests/library/scenario
    alice/tests/library/service
    alice/tests/library/solomon
    alice/tests/library/surface
    alice/tests/library/uniclient
    alice/tests/library/url
    alice/tests/library/vault
    alice/tests/library/version
    alice/tests/library/vins_response
    alice/tests/library/ydb
    contrib/python/flaky
    contrib/python/pytz
    contrib/python/requests
)

DATA(arcadia/alice/tests/data)

IF(AP OR ALL)
    DEPENDS(
        alice/hollywood/library/python/testing/app_host
    )
    DATA(
        arcadia/apphost/conf
    )
ENDIF()

IF(HW OR ALL)
    INCLUDE(${ARCADIA_ROOT}/alice/hollywood/shards/all/for_it2.inc)
ENDIF()

IF(MM OR ALL)
    DEPENDS(
        alice/megamind/scripts/run
        alice/megamind/server
    )
    DATA(
        arcadia/alice/megamind/configs/common/classification.pb.txt
        arcadia/alice/megamind/configs/dev/combinators
        arcadia/alice/megamind/configs/dev/megamind.pb.txt
        arcadia/alice/megamind/configs/dev/scenarios
    )
ENDIF()

TAG(ya:external ya:fat ya:force_sandbox ya:not_autocheck)

REQUIREMENTS(
    sb_vault:YAV_TOKEN=file:mihajlova:yav-alice-integration-tests-token
    network:full
)

RESOURCE(
    mordovia/resources/mordovia_video_selection_device_state.json mordovia_video_selection_device_state.json
    notifications/messages/music_push.txt music_push.txt
)

END()
