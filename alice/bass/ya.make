LIBRARY()

OWNER(g:bass)

CFLAGS(
    -DPIRE_NO_CONFIG
)

PEERDIR(
    alice/bass/common_nlg
    alice/bass/forms/external_skill/proto
    alice/bass/forms/video/protocol_scenario_helpers
    alice/bass/libs/analytics
    alice/bass/libs/app_host
    alice/bass/libs/avatars
    alice/bass/libs/client
    alice/bass/libs/config
    alice/bass/libs/eventlog
    alice/bass/libs/facts
    alice/bass/libs/fetcher
    alice/bass/libs/forms_db
    alice/bass/libs/globalctx
    alice/bass/libs/logging_v2
    alice/bass/libs/metrics
    alice/bass/libs/ner
    alice/bass/libs/push_notification
    alice/bass/libs/radio
    alice/bass/libs/request
    alice/bass/libs/rtlog
    alice/bass/libs/rule_matcher
    alice/bass/libs/scheduler
    alice/bass/libs/smallgeo
    alice/bass/libs/socialism
    alice/bass/libs/source_request
    alice/bass/libs/tvm2
    alice/bass/libs/video_common
    alice/bass/libs/video_common/parsers
    alice/bass/libs/ydb_config
    alice/bass/libs/ydb_helpers
    alice/bass/util
    alice/library/analytics/common
    alice/library/app_navigation
    alice/library/billing
    alice/library/biometry
    alice/library/blackbox
    alice/library/calendar_parser
    alice/library/client
    alice/library/datetime
    alice/library/experiments
    alice/library/geo
    alice/library/geo_resolver
    alice/library/json
    alice/library/music
    alice/library/network
    alice/library/onboarding
    alice/library/parsed_user_phrase
    alice/library/passport_api
    alice/library/proto
    alice/library/response_similarity
    alice/library/restriction_level
    alice/library/scenarios/alarm
    alice/library/scenarios/reminders
    alice/library/scled_animations
    alice/library/skill_discovery
    alice/library/special_location
    alice/library/url_builder
    alice/library/util
    alice/library/video_common
    alice/library/video_common/restreamed_data
    alice/library/websearch
    alice/megamind/library/search/protos
    alice/megamind/library/util
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/nlu/libs/request_normalizer
    alice/protos/data
    alice/rtlog/client
    contrib/libs/protobuf
    contrib/libs/protoc
    dict/lang_detector/libs/core
    dict/libs/fst/lib
    dict/misspell/spellchecker/lib/kernel
    dict/mt/libs/libmt
    dict/translit/libs/translit
    dj/services/alisa_skills/server/proto/client
    extsearch/geo/kernel/normalize
    kernel/alice/music_scenario/web_url_canonizer/lib
    kernel/formula_storage
    kernel/geodb
    kernel/inflectorlib/phrase/simple
    kernel/lemmer/core
    kernel/reqid
    kernel/translit
    kernel/urlnorm
    library/cpp/containers/stack_vector
    library/cpp/deprecated/atomic
    library/cpp/expression
    library/cpp/geobase
    library/cpp/geolocation
    library/cpp/getopt
    library/cpp/html/dehtml
    library/cpp/http/io
    library/cpp/http/server
    library/cpp/iterator
    library/cpp/json
    library/cpp/json/writer
    library/cpp/langs
    library/cpp/neh
    library/cpp/protobuf/json
    library/cpp/regex/pire
    library/cpp/scheme
    library/cpp/scheme/util
    library/cpp/semver
    library/cpp/sighandler
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/telfinder
    library/cpp/text_processing/tokenizer
    library/cpp/threading/future
    library/cpp/threading/hot_swap
    library/cpp/timezone_conversion
    library/cpp/tokenizer
    library/cpp/tvmauth
    library/cpp/tvmauth/client
    library/cpp/xml/document
    maps/doc/proto/yandex/maps/proto/common2
    maps/doc/proto/yandex/maps/proto/driving
    maps/doc/proto/yandex/maps/proto/masstransit
    maps/doc/proto/yandex/maps/proto/search
    search/alice/serp_summarizer/runtime/proto
    search/idl
    search/session/compression
    web/src_setup/lib/setup/images_cbir_postprocess/intents_classifier/intents
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
    ysite/yandex/reqanalysis
    yweb/protos/lib
    # TODO: Move to alice/library
)

SRCS(
    application.cpp
    http_request.cpp
    ping_handler.cpp
    push_handler.cpp
    reopenlogs_handler.cpp
    server.cpp
    clients.cpp
    setup/setup.cpp
    version_handler.cpp
    # bass
    forms/battery_power_state.cpp
    forms/thereminvox.cpp
    forms/ether.cpp
    forms/ether/video.cpp
    forms/route.cpp
    forms/route_helpers.cpp
    forms/route_tools.cpp
    forms/poi.cpp
    forms/remember_address.cpp
    forms/afisha/afisha_proxy.cpp
    forms/automotive/automotive_demo_2019.cpp
    forms/automotive/fm_radio.cpp
    forms/automotive/greeting.cpp
    forms/automotive/media_control.cpp
    forms/automotive/open_site_or_app.cpp
    forms/automotive/sound.cpp
    forms/automotive/url_build.cpp
    forms/avia.cpp
    forms/bluetooth.cpp
    forms/browser_read_page.cpp
    forms/client_command.cpp
    forms/common/biometry_delegate.cpp
    forms/common/contacts/avatars.cpp
    forms/common/contacts/contacts.cpp
    forms/common/contacts/contacts.sc
    forms/common/contacts/contacts_finder.cpp
    forms/common/contacts/synonyms.cpp
    forms/common/contacts/translit.cpp
    forms/common/blackbox_api.cpp
    forms/common/data_sync_api.cpp
    forms/common/directives.cpp
    forms/common/personal_data.cpp
    forms/common/saved_address.cpp
    forms/common/saved_address.sc
    forms/common/uid_utils.cpp
    forms/computer_vision/answers.cpp
    forms/computer_vision/context.cpp
    forms/computer_vision/handler.cpp
    forms/computer_vision/barcode_handler.cpp
    forms/computer_vision/translate_handler.cpp
    forms/computer_vision/similarlike_cv_handler.cpp
    forms/computer_vision/similar_artwork_handler.cpp
    forms/computer_vision/info_cv_handler.cpp
    forms/computer_vision/market_handler.cpp
    forms/computer_vision/clothes_handler.cpp
    forms/computer_vision/ocr_handler.cpp
    forms/computer_vision/office_lens_handler.cpp
    forms/computer_vision/ocr_voice_handler.cpp
    forms/context/context.cpp
    forms/context/fwd.cpp
    forms/continuation_register.cpp
    forms/continuations.cpp
    forms/convert.cpp
    forms/crmbot/forms.cpp
    forms/crmbot/order_id.cpp
    forms/crmbot/personal_data_helper.cpp
    forms/crmbot/clients/checkouter.sc
    forms/crmbot/clients/checkouter.cpp
    forms/crmbot/clients/report.cpp
    forms/crmbot/clients/report_url_redirect.sc
    forms/crmbot/handlers/authorizing_handler.cpp
    forms/crmbot/handlers/base_handler.cpp
    forms/crmbot/handlers/order_cancel_handler.cpp
    forms/crmbot/handlers/order_cancel_we_did_handler.cpp
    forms/crmbot/handlers/order_status_handler.cpp
    forms/crmbot/handlers/search_handler.cpp
    forms/directives.inc
    GLOBAL forms/directives.cpp
    forms/geoaddr.cpp
    forms/geocoder.cpp
    forms/geodb.cpp
    forms/get_location.cpp
    forms/geo_resolver.cpp
    forms/handlers.cpp
    forms/happy_new_year.cpp
    forms/external_skill.cpp
    forms/external_skill/abuse.cpp
    forms/external_skill/discovery.cpp
    forms/external_skill/dj_entry_point.cpp
    forms/external_skill/enums.cpp
    forms/external_skill/error.cpp
    forms/external_skill/ifs_map.cpp
    forms/external_skill/skill.cpp
    forms/external_skill/parser1x.cpp
    forms/external_skill/scheme.sc
    forms/external_skill/util.cpp
    forms/external_skill/validator.cpp
    forms/external_skill_recommendation/enums.cpp
    forms/external_skill_recommendation/card_block.sc
    forms/external_skill_recommendation/service_response.sc
    forms/external_skill_recommendation/skill_recommendation.cpp
    forms/facts.cpp
    forms/general_conversation/general_conversation.cpp
    forms/general_conversation/general_conversation_vinsless.cpp
    forms/fairy_tales.cpp
    forms/feedback.cpp
    forms/games_onboarding.cpp
    forms/getdate.cpp
    forms/gettime.cpp
    forms/news/news.cpp
    forms/news/newsdata.cpp
    forms/no_op.cpp
    forms/maps_static_api.cpp
    forms/market.cpp
    forms/market/base_market_choice.cpp
    forms/market/bass_response.sc
    forms/market/checkout_user_info.cpp
    forms/market/clear_request.cpp
    forms/market/client.cpp
    forms/market/client/base.cpp
    forms/market/client/base_redirect.cpp
    forms/market/client/checkout.sc
    forms/market/client/checkouter_client.cpp
    forms/market/client/formalizer.cpp
    forms/market/client/geo_client.cpp
    forms/market/client/mds.sc
    forms/market/client/model_redirect.cpp
    forms/market/client/parametric_redirect.cpp
    forms/market/client/redirect.cpp
    forms/market/client/region_redirect.cpp
    forms/market/client/pers_basket_client.cpp
    forms/market/client/pers_basket_entry.cpp
    forms/market/client/report.cpp
    forms/market/client/report.sc
    forms/market/client/report_client.cpp
    forms/market/client/result.cpp
    forms/market/client/stock_storage.sc
    forms/market/client/stock_storage_client.cpp
    forms/market/client/tsum_tracer.cpp
    forms/market/market_checkout.cpp
    forms/market/context.cpp
    alice/bass/forms/market/delivery_builder.cpp
    forms/market/delivery_intervals_worker.cpp
    forms/market/dynamic_data.cpp
    forms/market/experiments.cpp
    forms/market/fake_users.cpp
    forms/market/filter_worker.cpp
    forms/market/forms.cpp
    forms/market/login.cpp
    forms/market/market_choice.cpp
    forms/market/market_geo_support.cpp
    forms/market/market_how_much_impl.cpp
    forms/market/market_orders_status.cpp
    forms/market/market_recurring_purchase.cpp
    forms/market/market_url_builder.cpp
    forms/market/number_filter_worker.cpp
    forms/market/product_offers_card.cpp
    forms/market/shopping_list.cpp
    forms/market/client/beru_bonuses.sc
    forms/market/client/beru_bonuses_client.cpp
    forms/market/market_beru_my_bonuses_list.cpp
    forms/market/settings.cpp
    forms/market/slots.sc
    forms/market/types/filter.cpp
    forms/market/types/model.cpp
    forms/market/types/offer.cpp
    forms/market/types/picture.cpp
    forms/market/types/promotions.cpp
    forms/market/types/warning.cpp
    forms/market/util/amount.cpp
    forms/market/util/report.cpp
    forms/market/util/serialize.cpp
    forms/market/util/string.cpp
    forms/market/util/suggests.cpp
    forms/messaging.cpp
    forms/music/ambient_sound.cpp
    forms/music/base_music_answer.cpp
    forms/music/base_provider.cpp
    forms/music/cache.cpp
    forms/music/catalog.cpp
    forms/music/converter.cpp
    forms/music/fairy_tales.cpp
    forms/music/music.cpp
    forms/music/music_anaphora.cpp
    forms/music/push_message.cpp
    forms/music/quasar_answer.cpp
    forms/music/quasar_provider.cpp
    forms/music/sing_song.cpp
    forms/music/snippet_provider.cpp
    forms/music/websearch.cpp
    forms/music/yamusic_answer.cpp
    forms/music/yamusic_provider.cpp
    forms/music/yaradio_answer.cpp
    forms/music/yaradio_provider.cpp
    forms/navigation/bno_apps.cpp
    forms/navigation/fixlist.cpp
    forms/navigation/navigation.cpp
    forms/navigator/add_point.cpp
    forms/navigator/bookmarks_matcher.cpp
    forms/navigator/faster_route.cpp
    forms/navigator/how_long.cpp
    forms/navigator/navigator_intent.cpp
    forms/navigator/navi_onboarding.cpp
    forms/navigator/map_search_intent.cpp
    forms/navigator/route_intents.cpp
    forms/navigator/set_sound.cpp
    forms/navigator/simple_intents.cpp
    forms/navigator/show_on_map_intent.cpp
    forms/navigator/user_bookmarks.cpp
    forms/navigator/refuel.cpp
    forms/onboarding.cpp
    forms/parallel_handler.cpp
    forms/phone_call/phone_call.cpp
    forms/player/player.cpp
    forms/player_command.cpp
    forms/player_command/player_command.sc
    forms/player_command/continue.cpp
    forms/player_command/authorized.cpp
    forms/player_command/next_prev.cpp
    forms/player_command/simple.cpp
    forms/player_command/rewind.cpp
    forms/protocol_scenario/protocol_scenario_utils.cpp
    forms/radio.cpp
    forms/random_number.cpp
    forms/registrator.cpp
    forms/request.cpp
    forms/search/film_gallery.cpp
    forms/search/direct_continuation.cpp
    forms/search/direct_gallery.cpp
    forms/search/search.cpp
    forms/search/serp.cpp
    forms/search/serp_gallery.cpp
    forms/search/serp_gallery_builder.cpp
    forms/send_bugreport.cpp
    forms/session_start.cpp
    forms/setup_context.cpp
    forms/shazam.cpp
    forms/show_collection.cpp
    forms/search_filter.cpp
    forms/sound.cpp
    forms/special_location.cpp
    forms/tanker.sc
    forms/taxi/handler.cpp
    forms/taxi/integration_api.cpp
    forms/taxi.cpp
    forms/reminders/alarm.cpp
    forms/reminders/constants.cpp
    forms/reminders/device_reminders.cpp
    forms/reminders/helpers.cpp
    forms/reminders/memento_reminders.cpp
    forms/reminders/registrator.cpp
    forms/reminders/reminder.cpp
    forms/reminders/request.cpp
    forms/reminders/timer.cpp
    forms/reminders/todo.cpp
    forms/traffic.cpp
    forms/translate/translate.cpp
    forms/tv/channels_info.cpp
    forms/tv/onboarding.cpp
    forms/tv/tv_broadcast.cpp
    forms/tv/tv_helper.cpp
    forms/tv/tv_next_episode.cpp
    forms/tv/switch_input.cpp
    forms/unit_test_form.cpp
    forms/urls_builder.cpp
    forms/user_aware_handler.cpp
    forms/video/video.cpp
    forms/video/amediateka_provider.cpp
    forms/video/billing.cpp
    forms/video/billing_api.cpp
    forms/video/billing_new.cpp
    forms/video/change_track.cpp
    forms/video/change_track_hardcoded.cpp
    forms/video/defs.cpp
    forms/video/easter_eggs.cpp
    forms/video/entity_search.cpp
    forms/video/environment_state.cpp
    forms/video/items.cpp
    forms/video/ivi_provider.cpp
    forms/video/void_provider.cpp
    forms/video/kinopoisk_content_snapshot.cpp
    forms/video/kinopoisk_provider.cpp
    forms/video/kinopoisk_recommendations.cpp
    forms/video/mordovia_webview_settings.cpp
    forms/video/okko_provider.cpp
    forms/video/onboarding.cpp
    forms/video/player_command.cpp
    forms/video/play_video.cpp
    forms/video/push_message.cpp
    forms/video/requests.cpp
    forms/video/responses.cpp
    forms/video/show_video_settings.cpp
    forms/video/skip_fragment.cpp
    forms/video/utils.cpp
    forms/video/vh_player.cpp
    forms/video/video_how_long.cpp
    forms/video/video_provider.cpp
    forms/video/video_slots.cpp
    forms/video/video_command.cpp
    forms/video/web_os_helper.cpp
    forms/video/web_search.cpp
    forms/video/yavideo_provider.cpp
    forms/video/yavideo_proxy_provider.cpp
    forms/video/youtube_provider.cpp
    forms/vins.cpp
    forms/voiceprint/voiceprint_enroll.cpp
    forms/voiceprint/voiceprint_remove.cpp
    forms/watch/onboarding.cpp
    forms/weather/api.cpp
    forms/weather/current_weather.cpp
    forms/weather/day_hours_weather.cpp
    forms/weather/day_part_weather.cpp
    forms/weather/day_weather.cpp
    forms/weather/today_weather.cpp
    forms/weather/days_range_weather.cpp
    forms/weather/suggests_maker.cpp
    forms/weather/util.cpp
    forms/weather/weather_nowcast.cpp
    forms/weather/weather.cpp
    forms/whats_new.cpp
    protobuf_request.cpp
    test_users.cpp
    test_users_details.cpp
    ydb.cpp
    ydb_config.cpp
)

FROM_SANDBOX(FILE 725324844 OUT_NOAUTO geodb.bin)

FROM_SANDBOX(FILE 1223215759 OUT_NOAUTO avatars.json)

# Kinopoisk content snapshot ASSISTANT-2240 OTT-1393
FROM_SANDBOX(
    FILE 599710907 OUT_NOAUTO
        kinopoisk.json
        kinopoisk.json
)

FROM_SANDBOX(FILE 1666875119 OUT_NOAUTO news_lemmer.bin)

RESOURCE(
    news_lemmer.bin news_lemmer.bin
)

RESOURCE(
    avatars.json avatars_map
)

RESOURCE(
    geodb.bin geodb.data
)

RESOURCE(
    data/call_name_synonyms.txt call_name_synonyms.txt
)

RESOURCE(
    data/search_stubs.json search_stubs.json
)

RESOURCE(
    data/apple_schemes.json apple_schemes.json
)

RESOURCE(
    data/emergency_phones.json emergency_phones.json
)

RESOURCE(
    data/entity_search_jokes.json entity_search_jokes.json
)

RESOURCE(
    data/market_fake_users.txt market_fake_users.txt
)

RESOURCE(
    data/market_prefix_phrases.txt market_prefix_phrases.txt
)

RESOURCE(
    data/monsters_on_vacation_data.json monsters_on_vacation_data.json
)

RESOURCE(
    data/navigation_fixlist_general.json navigation_fixlist_general.json
)

RESOURCE(
    data/navigation_fixlist_turbo.json navigation_fixlist_turbo.json
)

RESOURCE(
    data/navigation_fixlist_yandex.json navigation_fixlist_yandex.json
)

RESOURCE(
    data/navigation_nativeapps.json navigation_nativeapps.json
)

RESOURCE(
    data/restreamed_channels.json restreamed_channels.json
)

RESOURCE(
    data/windows_fixlist.trie windows_fixlist.trie
)

RESOURCE(
    data/forms_sla.json forms_sla.json
)

RESOURCE(
    data/actions_sla.json actions_sla.json
)

RESOURCE(
    kinopoisk.json kinopoisk.json
)

RESOURCE(
    maps/automotive/radio/radio.json radio_station
)

# data necessary for translate scenario
FROM_SANDBOX(1329359460 OUT_NOAUTO lang_detector_model.arch)

FROM_SANDBOX(FILE 604536837 OUT_NOAUTO ru-numeral-yandex.yfst)

FROM_SANDBOX(FILE 568862754 OUT_NOAUTO ru.3gr)

FROM_SANDBOX(669834848 OUT_NOAUTO en-ru.translit.txt)

FROM_SANDBOX(569958158 OUT_NOAUTO en-ru.nonstandard.translit.txt)

FROM_SANDBOX(FILE 712707011 OUT_NOAUTO de-numeral-yandex.yfst)

FROM_SANDBOX(FILE 712825700 OUT_NOAUTO en-numeral-rhvoice.yfst)

FROM_SANDBOX(FILE 712834466 OUT_NOAUTO eo-numeral-rhvoice.yfst)

FROM_SANDBOX(FILE 712838395 OUT_NOAUTO es-numeral-yandex.yfst)

FROM_SANDBOX(FILE 712836245 OUT_NOAUTO fr-numeral-yandex.yfst)

FROM_SANDBOX(FILE 712841210 OUT_NOAUTO it-numeral-yandex.yfst)

FROM_SANDBOX(FILE 712845477 OUT_NOAUTO ka-numeral-rhvoice.yfst)

FROM_SANDBOX(FILE 712847207 OUT_NOAUTO ky-numeral-rhvoice.yfst)

FROM_SANDBOX(FILE 712849245 OUT_NOAUTO tr-numeral-yandex.yfst)

FROM_SANDBOX(FILE 712896369 OUT_NOAUTO tt-numeral-rhvoice.yfst)

FROM_SANDBOX(FILE 712901803 OUT_NOAUTO uk-numeral-yandex.yfst)

FROM_SANDBOX(FILE 731485431 OUT_NOAUTO ru-en.translit.txt)

FROM_SANDBOX(FILE 731487306 OUT_NOAUTO 1gr-lm.en-wiki.trie)

FROM_SANDBOX(FILE 916112353 OUT_NOAUTO swear.txt)

FROM_SANDBOX(FILE 1094753845 OUT_NOAUTO bad.txt)

RESOURCE(
    lang_detector_model.arch lang_detector_model
)

RESOURCE(
    ru-numeral-yandex.yfst ru_numeral_yandex
)

RESOURCE(
    de-numeral-yandex.yfst de_numeral_yandex
)

RESOURCE(
    en-numeral-rhvoice.yfst en_numeral_yandex
)

RESOURCE(
    eo-numeral-rhvoice.yfst eo_numeral_yandex
)

RESOURCE(
    es-numeral-yandex.yfst es_numeral_yandex
)

RESOURCE(
    fr-numeral-yandex.yfst fr_numeral_yandex
)

RESOURCE(
    it-numeral-yandex.yfst it_numeral_yandex
)

RESOURCE(
    ka-numeral-rhvoice.yfst ka_numeral_yandex
)

RESOURCE(
    ky-numeral-rhvoice.yfst ky_numeral_yandex
)

RESOURCE(
    tr-numeral-yandex.yfst tr_numeral_yandex
)

RESOURCE(
    tt-numeral-rhvoice.yfst tt_numeral_yandex
)

RESOURCE(
    uk-numeral-yandex.yfst uk_numeral_yandex
)

RESOURCE(
    ru.3gr rus.3gr
)

RESOURCE(
    en-ru.translit.txt en-ru.translit
)

RESOURCE(
    en-ru.nonstandard.translit.txt en-ru.nonstandard.translit
)

RESOURCE(
    ru-en.translit.txt ru-en.translit
)

RESOURCE(
    1gr-lm.en-wiki.trie names_lang_model
)

RESOURCE(
    swear.txt swear
)

RESOURCE(
    bad.txt cv_swear
)

RESOURCE(
    data/podcasts_top_filtred.json podcasts_top_filtred
)

RESOURCE(
    data/radio_stations.json radio_stations
)

RESOURCE(
    data/translate_language_stems.txt lang_stems
)

RESOURCE(
    data/translate_languages.txt languages
)

GENERATE_ENUM_SERIALIZATION(clients.h)

GENERATE_ENUM_SERIALIZATION(forms/avia.h)

GENERATE_ENUM_SERIALIZATION(forms/context/context.h)

GENERATE_ENUM_SERIALIZATION(forms/common/personal_data.h)

GENERATE_ENUM_SERIALIZATION(forms/computer_vision/context.h)

GENERATE_ENUM_SERIALIZATION(forms/crmbot/forms.h)

GENERATE_ENUM_SERIALIZATION(forms/crmbot/order_status.h)

GENERATE_ENUM_SERIALIZATION(forms/external_skill/enums.h)

GENERATE_ENUM_SERIALIZATION(forms/external_skill/skill.h)

GENERATE_ENUM_SERIALIZATION(forms/external_skill_recommendation/enums.h)

GENERATE_ENUM_SERIALIZATION(forms/market/client/tsum_tracer.h)

GENERATE_ENUM_SERIALIZATION(forms/market/context.h)

GENERATE_ENUM_SERIALIZATION(forms/market/forms.h)

GENERATE_ENUM_SERIALIZATION(forms/market/state.h)

GENERATE_ENUM_SERIALIZATION(forms/market/types.h)

GENERATE_ENUM_SERIALIZATION(forms/music/music.h)

GENERATE_ENUM_SERIALIZATION(forms/music/providers.h)

GENERATE_ENUM_SERIALIZATION(forms/news/news.h)

GENERATE_ENUM_SERIALIZATION(forms/player_command/defs.h)

GENERATE_ENUM_SERIALIZATION(forms/reminders/timer.h)

GENERATE_ENUM_SERIALIZATION(forms/urls_builder.h)

GENERATE_ENUM_SERIALIZATION(forms/video/billing.h)

GENERATE_ENUM_SERIALIZATION(forms/video/defs.h)

GENERATE_ENUM_SERIALIZATION(forms/video/ivi_provider.h)

GENERATE_ENUM_SERIALIZATION(forms/video/video_provider.h)

GENERATE_ENUM_SERIALIZATION(forms/taxi/statuses.h)

END()

RECURSE(
    bin
    common_nlg
    fixture
    forms/market/ut
    forms/music/ut
    libs
    scripts
    tools
    ut
)
