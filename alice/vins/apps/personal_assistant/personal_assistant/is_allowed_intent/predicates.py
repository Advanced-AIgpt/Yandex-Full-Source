from personal_assistant import clients
from personal_assistant import intents
from personal_assistant.is_allowed_intent.predicate_registrant import PredicateRegistrant

import logging

logger = logging.getLogger(__name__)

reg = PredicateRegistrant()


def get_current_screen(req_info):
    device_state = req_info.device_state or {}
    current_screen = device_state.get('video', {}).get('current_screen')
    return current_screen


def get_is_tv_plugged_in(req_info):
    device_state = req_info.device_state or {}
    is_tv_plugged_in = device_state.get('is_tv_plugged_in', False)
    return is_tv_plugged_in


@reg
def is_smart_speaker(req_info):
    return clients.is_smart_speaker(req_info.app_info)


@reg
def is_open_site_or_app(intent_name):
    return intents.is_open_site_or_app(intent_name)


@reg
def is_happy_new_year_bloggers_intent(intent_name):
    return intents.is_happy_new_year_bloggers_intent(intent_name)


@reg
def is_quasar_video_gallery_selection_by_text(intent_name):
    return intents.is_quasar_video_gallery_selection_by_text(intent_name)


@reg
def is_quasar_ether_gallery_selection_by_text(intent_name):
    return intents.is_quasar_ether_gallery_selection_by_text(intent_name)


@reg
def is_current_screen_not_in_gallery_season_gallery(req_info):
    return get_current_screen(req_info) not in ['gallery', 'season_gallery']


@reg
def is_current_screen_not_in_mordovia_gallery(req_info):
    return get_current_screen(req_info) not in ['mordovia_webview']


@reg
def is_quasar_channel_gallery_selection_by_text(intent_name):
    return intents.is_quasar_channel_gallery_selection_by_text(intent_name)


@reg
def is_current_screen_not_tv_gallery(req_info):
    return get_current_screen(req_info) != 'tv_gallery'


@reg
def is_quasar_tv_specific(intent_name):
    return intents.is_quasar_tv_specific(intent_name)


@reg
def is_tv_plugged_in(req_info):
    return get_is_tv_plugged_in(req_info)


@reg
def is_quasar_tv_specific_and_quasar_video_screen_selection(intent_name):
    return intents.is_quasar_tv_specific(intent_name) and intents.is_quasar_video_screen_selection(intent_name)


@reg
def is_quasar_tv_specific_and_quasar_gallery_navigation(intent_name):
    return intents.is_quasar_tv_specific(intent_name) and intents.is_quasar_gallery_navigation(intent_name)


@reg
def is_quasar_tv_specific_and_quasar_vertical_screen_navigation(intent_name):
    return intents.is_quasar_tv_specific(intent_name) and intents.is_quasar_vertical_screen_navigation(intent_name)


@reg
def is_quasar_tv_specific_and_quasar_video_payment_action(intent_name):
    return intents.is_quasar_tv_specific(intent_name) and intents.is_quasar_video_payment_action(intent_name)


@reg
def is_quasar_tv_specific_and_quasar_provider_authorization_action(intent_name):
    return intents.is_quasar_tv_specific(intent_name) and intents.is_quasar_provider_authorization_action(intent_name)


@reg
def is_quasar_tv_specific_and_quasar_video_details_opening_action(intent_name):
    return intents.is_quasar_tv_specific(intent_name) and intents.is_quasar_video_details_opening_action(intent_name)


@reg
def is_current_screen_not_in_main(req_info):
    return get_current_screen(req_info) not in ['main']


@reg
def is_current_screen_not_in_gallery_season_gallery_tv_gallery_mordovia(req_info):
    return get_current_screen(req_info) not in ['gallery', 'season_gallery', 'tv_gallery', 'mordovia_webview']


@reg
def is_current_screen_not_in_gallery_season_gallery_tv_gallery_mordovia_allowed_tv_screens(req_info):
    return (is_current_screen_not_in_gallery_season_gallery_tv_gallery_mordovia(req_info) and
            is_current_screen_not_in_tv_screens_allowed_for_vertical_navigation(req_info))


@reg
def is_current_screen_not_in_mordovia(req_info):
    return get_current_screen(req_info) not in ['mordovia_webview']


@reg
def is_current_screen_not_in_description_season_gallery(req_info):
    return get_current_screen(req_info) not in ['description', 'season_gallery']


@reg
def is_current_screen_not_in_description(req_info):
    return get_current_screen(req_info) not in ['description']


@reg
def is_current_screen_not_in_description_video_player_payment_season_gallery(req_info):
    return get_current_screen(req_info) not in ['description', 'video_player', 'payment', 'season_gallery']


@reg
def is_current_screen_not_in_tv_screens_allowed_for_vertical_navigation(req_info):
    return get_current_screen(req_info) not in ['search_results', 'content_details', 'tv_expanded_collection']


@reg
def is_current_screen_not_in_mordovia_and_not_in_tv_screens_allowed_for_vertical_navigation(req_info):
    return is_current_screen_not_in_mordovia(req_info) and is_current_screen_not_in_tv_screens_allowed_for_vertical_navigation(req_info)


@reg
def is_alarm_stop_playing_intent(intent_name):
    return intents.is_alarm_stop_playing_intent(intent_name)


@reg
def is_alarm_snooze_intent(intent_name):
    return intents.is_alarm_snooze_intent(intent_name)


@reg
def is_alarm_buzzing(req_info):
    return intents.is_alarm_buzzing(req_info)


@reg
def is_alarm_sound_set_level_intent(intent_name):
    return intents.is_alarm_sound_set_level_intent(intent_name)


@reg
def has_alarm_in_utterance(req_info):
    return req_info.utterance is not None and 'будильник'.decode('utf-8') in unicode(req_info.utterance)


@reg
def is_timer_stop_playing_intent(intent_name):
    return intents.is_timer_stop_playing_intent(intent_name)


@reg
def is_timer_buzzing(req_info):
    return intents.is_timer_buzzing(req_info)


@reg
def is_stroka_intent(intent_name):
    return intents.is_stroka_intent(intent_name)


@reg
def is_not_stroka_or_yabro(req_info):
    client_is_stroka = clients.is_stroka(req_info.app_info)
    client_is_yabro = (clients.is_yabro_windows(req_info.app_info) and
                       req_info.experiments['stroka_yabro'] is not None)
    return not (client_is_stroka or client_is_yabro)


@reg
def is_player_intent(intent_name):
    return intents.is_player_intent(intent_name)


@reg
def does_have_player(req_info):
    return clients.has_player(req_info)


@reg
def is_player_intent_and_not_player_resume_intent(intent_name):
    return intents.is_player_intent(intent_name) and not intents.is_player_resume_intent(intent_name)


@reg
def player_commands_are_disabled_and_not_auto_and_not_after_shazam(session, req_info):
    return not (intents.are_player_commands_enabled(req_info) or clients.is_auto(req_info.app_info)
                or session.intent_name in intents.SHAZAM_INTENTS)


@reg
def is_player_replay_intent(intent_name):
    return intents.is_player_replay_intent(intent_name)


@reg
def is_smart_speaker_and_tv_plugged_in_and_is_current_screen_not_in_video_player_music_player_radio_player(req_info):
    return clients.is_smart_speaker(req_info.app_info) and get_is_tv_plugged_in(req_info) and get_current_screen(
        req_info) not in ['video_player', 'music_player', 'radio_player']


@reg
def is_smart_speaker_and_not_tv_plugged_in_and_not_really_playing(req_info):
    return clients.is_smart_speaker(req_info.app_info) and not get_is_tv_plugged_in(
        req_info) and not intents.is_really_playing(req_info)


@reg
def is_sound_intent_and_not_is_sound_toggle_intent(intent_name):
    return intents.is_sound_intent(intent_name) and not intents.is_sound_toggle_intent(intent_name)


@reg
def can_change_sound(req_info):
    return intents.can_change_sound(req_info)


@reg
def is_sound_toggle_intent(intent_name):
    return intents.is_sound_toggle_intent(intent_name)


@reg
def can_toggle_sound(req_info):
    return intents.can_toggle_sound(req_info)


@reg
def is_navigator_intent(intent_name):
    return intents.is_navigator_intent(intent_name)


@reg
def is_not_client_with_navigator(req_info):
    return not clients.is_client_with_navigator(req_info)


@reg
def is_req_info(req_info):
    return req_info


@reg
def is_quasar(req_info):
    return clients.is_quasar(req_info.app_info)


@reg
def is_tv_stream_intent(intent_name):
    return intents.is_tv_stream_intent(intent_name)


@reg
def is_smart_speaker_and_not_tv_plugged_in(req_info):
    return clients.is_smart_speaker(req_info.app_info) and not get_is_tv_plugged_in(req_info)


@reg
def is_quasar_faq(intent_name):
    return intents.is_quasar_faq(intent_name)


@reg
def is_not_smart_speaker(req_info):
    return not clients.is_smart_speaker(req_info.app_info)


@reg
def is_onboarding_continuation_intent(intent_name):
    return intents.is_onboarding_continuation_intent(intent_name)


@reg
def is_not_smart_speaker_or_elari_watch_or_tv(req_info):
    return not (clients.is_smart_speaker(req_info.app_info) or clients.is_elari_watch(req_info.app_info) or clients.is_tv_device(req_info.app_info))


@reg
def is_player_intent_or_sound_intent(intent_name):
    return intents.is_player_intent(intent_name) or intents.is_sound_intent(intent_name)


@reg
def is_elari_watch(req_info):
    return clients.is_elari_watch(req_info.app_info)


@reg
def is_sleep_timer_intent(intent_name):
    return intents.is_sleep_timer_intent(intent_name)


@reg
def req_info_experiments_sleep_timers_is_none(req_info):
    return req_info.experiments['sleep_timers'] is None


@reg
def is_taxi_intent(intent_name):
    return intents.is_taxi_intent(intent_name)


@reg
def is_taxi_new_experiment_not_none(req_info):
    return req_info.experiments['disable_taxi_new'] is None


@reg
def is_taxi_new_intent(intent_name):
    return intents.is_taxi_new_intent(intent_name)


@reg
def is_taxi_new_experiment_none(req_info):
    return not is_taxi_new_experiment_not_none(req_info)


@reg
def is_my_name(intent_name):
    return intents.is_my_name(intent_name)


@reg
def is_personalization_experiment_none(req_info):
    return req_info.experiments['personalization'] is None


@reg
def is_market_native(intent_name):
    return intents.is_market_native(intent_name)


@reg
def is_market_native_experiment_none(req_info):
    return req_info.experiments['market_native'] is None


@reg
def is_market_intent(intent_name):
    return intents.is_market_intent(intent_name)


@reg
def is_market_disable_experiment_not_none_and_market_enable_experiment_none(req_info):
    return req_info.experiments['market_disable'] is not None and req_info.experiments["market_enable"] is None


@reg
def is_market_beru_intent(intent_name):
    return intents.is_market_beru_intent(intent_name)


@reg
def is_market_beru_disable_experiment_none(req_info):
    return req_info.experiments['market_beru_disable'] is None


@reg
def is_recurring_purchase_intent(intent_name):
    return intents.is_recurring_purchase_intent(intent_name)


@reg
def is_recurring_purchase_disable_experiment_none(req_info):
    return req_info.experiments['recurring_purchase_disable'] is None


@reg
def is_shopping_list_intent(intent_name):
    return intents.is_shopping_list_intent(intent_name)


@reg
def is_shopping_list_fixlist_intent(intent_name):
    return intents.is_shopping_list_fixlist_intent(intent_name)


@reg
def is_shopping_list_disable_experiment_none(req_info):
    return req_info.experiments['shopping_list_disable'] is None


@reg
def is_shopping_list_knn_disable_experiment_none(req_info):
    return req_info.experiments['shopping_list_knn_disable'] is None


@reg
def is_shopping_list_experiment_none(req_info):
    return req_info.experiments["shopping_list"] is None


@reg
def is_shopping_list_fixlist_experiment_none(req_info):
    return req_info.experiments["shopping_list_fixlist"] is None


@reg
def is_how_much_intent(intent_name):
    return intents.is_how_much_intent(intent_name)


@reg
def is_how_much_disable_experiment_none(req_info):
    return req_info.experiments['how_much_disable'] is None


@reg
def is_how_much_internal_intent(intent_name):
    return intents.is_how_much_internal_intent(intent_name)


@reg
def is_protocol_how_much_disable_experiment_none(req_info):
    return req_info.experiments["mm_disable_protocol_scenario=MarketHowMuch"] is None


@reg
def is_video_play_intent(intent_name):
    return intents.is_video_play_intent(intent_name)


@reg
def is_video_play_conditions_not_achieved(req_info):
    if req_info.experiments['video_play'] is not None:
        return False
    if is_smart_speaker(req_info):
        return False
    return True


@reg
def is_skill(intent_name):
    return intents.is_skill(intent_name)


@reg
def is_no_external_skill_experiment_not_none(req_info):
    return req_info.experiments['no_external_skill'] is not None


@reg
def is_translate_intent(intent_name):
    return intents.is_translate_intent(intent_name)


@reg
def is_translate_experiment_none(req_info):
    return req_info.experiments['translate'] is None


@reg
def is_refuel_intent(intent_name):
    return intents.is_refuel_intent(intent_name)


@reg
def is_refuel_experiment_none(req_info):
    return req_info.experiments['refuel'] is None


@reg
def is_avia_intent(intent_name):
    return intents.is_avia_intent(intent_name)


@reg
def is_avia_experiment_none(req_info):
    return req_info.experiments['avia'] is None


@reg
def is_show_collection_intent(intent_name):
    return intents.is_show_collection_intent(intent_name)


@reg
def is_show_collection_experiment_none(req_info):
    return req_info.experiments['show_collection'] is None


@reg
def is_happy_new_year_intent(intent_name):
    return intents.is_happy_new_year_intent(intent_name)


@reg
def is_happy_new_year_experiment_none(req_info):
    return req_info.experiments['happy_new_year'] is None


@reg
def is_radio_onboarding_intent(intent_name):
    return intents.is_radio_onboarding_intent(intent_name)


@reg
def are_radio_play_experiments_none(req_info):
    return req_info.experiments['radio_play_in_quasar'] is None and req_info.experiments['radio_play_in_search'] is None


@reg
def is_voiceprint_remove(intent_name):
    return intents.is_voiceprint_remove(intent_name)


@reg
def is_biometry_remove_experiment_none(req_info):
    return req_info.experiments['biometry_remove'] is None


@reg
def is_alarm_sound_old_microintent(intent_name):
    return intents.is_alarm_sound_old_microintent(intent_name)


@reg
def is_change_alarm_sound_experiment_none(req_info):
    return req_info.experiments['change_alarm_sound'] is None


@reg
def is_drive_faq(intent_name):
    return intents.is_drive_faq(intent_name)


@reg
def is_yandex_drive(req_info):
    return clients.is_yandex_drive(req_info)


@reg
def is_safe_for_autoapp(intent_name):
    return intents.is_safe_for_autoapp(intent_name)


@reg
def is_autoapp(req_info):
    return clients.is_autoapp(req_info.app_info)


@reg
def is_searchapp(req_info):
    return clients.is_searchapp(req_info.app_info)


@reg
def is_autoapp_microintent(intent_name):
    return intents.is_autoapp_microintent(intent_name)


@reg
def is_allowed_in_gc(intent_name):
    return intents.is_allowed_in_gc(intent_name)


@reg
def is_pure_general_conversation_in_session(session):
    return True if session.get('pure_general_conversation') else False


@reg
def is_gc_only(intent_name):
    return intents.is_gc_only(intent_name)


@reg
def is_handcrafted_deaf_intent(intent_name):
    return intents.is_handcrafted_deaf_intent(intent_name)


@reg
def supports_vertical_screen_navigation(req_info):
    return clients.supports_vertical_screen_navigation(req_info)
