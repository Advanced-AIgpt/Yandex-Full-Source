# coding: utf-8
from vins_core.nlu.flow_nlu_factory.transition_model import register_transition_model
from vins_core.nlg.template_nlg import register_filter, register_global, register_test
from vins_core.nlu.token_classifier import register_token_classifier_type
from vins_core.nlu.input_source_classifier import UtteranceInputSourceClassifier
from vins_core.dm.form_filler.feature_updater import register_feature_updater

from personal_assistant import nlg_filters, nlg_globals, skills
from personal_assistant.transition_model import create_pa_transition_model
from personal_assistant.hardcoded_responses import HardcodedResponseLookupTokenClassifier
from personal_assistant.setup_feature_updater import BassSetupFeatureUpdater
from personal_assistant.soft_force_intents import SoftForceIntentsClassifier

# transition model
register_transition_model('personal_assistant', create_pa_transition_model)

# classifiers
register_token_classifier_type(HardcodedResponseLookupTokenClassifier, 'hc_lookup')
register_token_classifier_type(SoftForceIntentsClassifier, 'soft_force_intents')
register_token_classifier_type(UtteranceInputSourceClassifier, 'utterance_input_source_lookup')

# NLG filters
register_filter('pluralize_tag', nlg_filters.pluralize_tag)
register_filter('inflect_amount_of_money', nlg_filters.inflect_amount_of_money)
register_filter('choose_equal_verb', nlg_filters.choose_equal_verb)
register_filter('try_round_float', nlg_filters.try_round_float)
register_filter('split_cost_with_spaces', nlg_filters.split_cost_with_spaces)
register_filter('normalize_time_units', nlg_filters.normalize_time_units)
register_filter('ceil_seconds', nlg_filters.ceil_seconds)
register_filter('alarm_time_format', nlg_filters.alarm_time_format)
register_filter('music_title_shorten', nlg_filters.music_title_shorten)
register_filter('strip', nlg_filters.strip)
register_filter('number_to_word', nlg_filters.number_to_word)
register_filter('render_date_with_on_preposition', nlg_filters.render_date_with_on_preposition)
register_filter('image_ratio', nlg_filters.image_ratio)

# NLG globals
register_global('is_stroka', nlg_globals.is_stroka)
register_global('is_telegram', nlg_globals.is_telegram)
register_global('is_smart_speaker', nlg_globals.is_smart_speaker)
register_global('is_quasar', nlg_globals.is_quasar)
register_global('is_legatus', nlg_globals.is_legatus)
register_global('is_mini_speaker', nlg_globals.is_mini_speaker)
register_global('is_mini_speaker_dexp', nlg_globals.is_mini_speaker_dexp)
register_global('is_mini_speaker_lg', nlg_globals.is_mini_speaker_lg)
register_global('is_mini_speaker_elari', nlg_globals.is_mini_speaker_elari)
register_global('has_uncontrollable_updates', nlg_globals.has_uncontrollable_updates)
register_global('is_desktop', nlg_globals.is_desktop)
register_global('is_ios', nlg_globals.is_ios)
register_global('is_android', nlg_globals.is_android)
register_global('is_searchapp_android', nlg_globals.is_searchapp_android)
register_global('is_searchapp_ios', nlg_globals.is_searchapp_ios)
register_global('is_searchapp', nlg_globals.is_searchapp)
register_global('is_active_attention', nlg_globals.is_active_attention)
register_global('is_navigator', nlg_globals.is_navigator)
register_global('is_navigator_projected_mode', nlg_globals.is_navigator_projected_mode)
register_global('is_client_with_navigator', nlg_globals.is_client_with_navigator)
register_global('is_elari_watch', nlg_globals.is_elari_watch)
register_global('is_webtouch', nlg_globals.is_webtouch)
register_global('is_gc_skill', nlg_globals.is_gc_skill)
register_global('is_tv_plugged_in', nlg_globals.is_tv_plugged_in)
register_global('is_yandex_drive', nlg_globals.is_yandex_drive)
register_global('is_auto', nlg_globals.is_auto)
register_global('is_auto_kaptur', nlg_globals.is_auto_kaptur)
register_global('is_ya_music_app', nlg_globals.is_ya_music_app)
register_global('is_yabro_windows', nlg_globals.is_yabro_windows)
register_global('is_yabro_desktop', nlg_globals.is_yabro_desktop)
register_global('is_yabro_mobile_android', nlg_globals.is_yabro_mobile_android)
register_global('is_yabro_mobile_ios', nlg_globals.is_yabro_mobile_ios)
register_global('is_tv_device', nlg_globals.is_tv_device)
register_global('is_sdg', nlg_globals.is_sdg)
register_global('has_alicesdk_player', nlg_globals.has_alicesdk_player)
register_global('get_attention', nlg_globals.get_attention)
register_global('get_commands', nlg_globals.get_commands)
register_global('is_yandex_spotter', nlg_globals.is_yandex_spotter)
register_global('is_child_request', nlg_globals.is_child_request)
register_global('is_child_content_settings', nlg_globals.is_child_content_settings)
register_global('is_child_microintent', nlg_globals.is_child_microintent)
register_global('assistant_name', nlg_globals.assistant_name)
register_global('get_device_id', nlg_globals.get_device_id)
register_global('username_suffix', nlg_globals.username_suffix)
register_global('username_infix', nlg_globals.username_infix)
register_global('username_prefix', nlg_globals.username_prefix)
register_global('username_prefix_if_needed', nlg_globals.username_prefix_if_needed)
register_global('content_restriction', nlg_globals.content_restriction)
register_global('add_hours', nlg_globals.add_hours)
register_global('render_traffic_forecast', nlg_globals.render_traffic_forecast)
register_global('geodesic_distance', nlg_globals.geodesic_distance)


# NLG tests
register_test('gc_skill', skills.is_gc_skill)
register_test('total_dictation', skills.is_total_dictation)

# feature updater
register_feature_updater(BassSetupFeatureUpdater)
