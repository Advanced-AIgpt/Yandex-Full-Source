from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings, TimingsResolution
from alice.uniproxy.library.perf_tester import events

YALDI_PARTIAL_HGRAM = 'yaldi_partial'
YABIO_PARTIAL_HGRAM = 'yabio_partial'
CLASSIFY_PARTIAL_HGRAM = 'classify_partial'
VINS_REQUEST_HGRAM = 'vins_request'
VINS_APPLY_REQUEST_DURATION_HGRAM = 'vins_apply_request_duration'
USEFUL_VINS_APPLY_PREPEARE_REQUEST_DURATION = 'useful_vins_apply_prepare_request_duration'
USEFUL_VINS_APPLY_REQUEST_DURATION_HGRAM = 'useful_vins_apply_request_duration'
RESPONSE_DELAY_HGRAM = 'response_delay'
TTS_REQUEST_HGRAM = 'tts_request'
TTS_FIRST_CHUNK_HGRAM = 'tts_first_chunk'
BIOMETRY_SCORING_CONFIDENCE_HGRAM = 'biometry_scoring_confidence'
ASR_MERGE_HGRAM = 'asr_merge'
ASR_MERGE_FIRST_RESPONSE_HGRAM = 'asr_merge_first_response'
ASR_MERGE_SECOND_RESPONSE_HGRAM = 'asr_merge_second_response'

VINS_LOAD_CONTEXT_HGRAM = 'vins_context_load'
VINS_SAVE_CONTEXT_HGRAM = 'vins_context_save'

CLIENT_VINS_RESPONSE_HGRAM = 'client_vins'
CLIENT_EOU_HGRAM = 'client_eou'
CLIENT_FIRST_MERGED_HGRAM = 'client_first_merged'
CLIENT_FIRST_PARTIAL_HGRAM = 'client_first_partial'
CLIENT_FIRST_SYNTH_HGRAM = 'client_first_synth'
CLIENT_LAST_PARTIAL_HGRAM = 'client_last_partial'
CLIENT_LAST_SYNTH_HGRAM = 'client_last_synth'

CONTACTS_REQUEST_HGRAM = 'contacts_request'
SMART_HOME_REQUEST_HGRAM = 'smart_home_request'
MEMENTO_GET_REQUEST_HGRAM = 'memento_get_request'
MEMENTO_UPDATE_REQUEST_HGRAM = 'memento_update_request'
DATASYNC_REQUEST_HGRAM = 'datasync_request'

VINS_REQUEST_SIZE_QUASAR_HGRAM = 'vins_request_quasar_size'
VINS_RESPONSE_SIZE_QUASAR_HGRAM = 'vins_response_quasar_size'
VINS_REQUEST_SIZE_OTHER_HGRAM = 'vins_request_other_size'
VINS_RESPONSE_SIZE_OTHER_HGRAM = 'vins_response_other_size'
VINS_RUN_WAIT_AFTER_EOU_HGRAM = 'vins_run_wait_after_eou'
VINS_RUN_DELAY_AFTER_EOU_HGRAM = 'vins_run_delay_after_eou'  # Zero if no wait actually happened.
VINS_RUN_WAIT_AFTER_EOU_HGRAM = 'vins_run_wait_after_eou'    # For actual wait only, always non-zero.


def asr_model_signals(model):
    """ Yields YASM signals for a given ASR model
    """
    for s in (
        "asr_{}_200_summ",
        "asr_{}_5xx_summ",
        "asr_{}_client_timeout_summ",
        "asr_{}_server_timeout_summ",
        "asr_{}_pumpkin_200_summ",
        "asr_{}_pumpkin_5xx_summ",
        "asr_{}_pumpkin_client_timeout_summ",
        "asr_{}_pumpkin_server_timeout_summ"
    ):
        yield s.format(model)


def asr_model_timings(model, boundaries):
    for s in (
        'asr_eou_eq_time_{}',
        'asr_eou_ne_time_{}',
        'asr_{}_partials'
    ):
        yield (s.format(model), boundaries)


def tts_model_signals(model):
    for s in (
        "tts_{}_200_summ",
        "tts_{}_err_summ",
        "tts_{}_fallback_200_summ",
        "tts_{}_fallback_err_summ"
    ):
        yield s.format(model)


class UniproxyCounter:
    _uniproxy_counters = [
        'asr_merge_first_summ',
        'asr_merge_second_summ',

        'asr_partial_eq_dialoggeneral_summ',
        'asr_partial_eq_dialoggeneral_gpu_summ',
        'asr_partial_eq_desktopgeneral_summ',
        'asr_partial_eq_quasargeneral_summ',
        'asr_partial_eq_quasargeneral_gpu_summ',
        'asr_partial_eq_dialogmaps_summ',
        'asr_partial_eq_dialogmaps_gpu_summ',

        'user_useful_vins_response_before_eou_summ',
        'quasar_user_useful_vins_response_before_eou_summ',
        'robot_useful_vins_response_before_eou_summ',
        'quasar_robot_useful_vins_response_before_eou_summ',

        *asr_model_signals("dialoggeneral"),
        *asr_model_signals("dialoggeneral_gpu"),
        *asr_model_signals("dialogmaps"),
        *asr_model_signals("dialogmaps_gpu"),
        *asr_model_signals("quasargeneral"),
        *asr_model_signals("quasargeneral_gpu"),
        *asr_model_signals("desktopgeneral"),

        'datasync_settings_ok_summ',
        'datasync_settings_err_summ',

        'cachalot_activation_fatal_error_summ',
        'cachalot_activation_first_error_summ',
        'cachalot_activation_first_request_summ',
        'cachalot_activation_second_allowed_summ',
        'cachalot_activation_second_another_is_better_summ',
        'cachalot_activation_second_error_summ',
        'cachalot_activation_second_leader_already_elected_summ',
        'cachalot_activation_second_request_summ',
        'cachalot_activation_third_allowed_summ',
        'cachalot_activation_third_another_is_better_summ',
        'cachalot_activation_third_error_summ',
        'cachalot_activation_third_leader_already_elected_summ',
        'cachalot_activation_third_loud_invalid_and_slow_summ',
        'cachalot_activation_third_request_summ',
        'cachalot_activation_third_unknown_leader_summ',
        'cachalot_activation_third_all_invalid_summ',

        'contacts_request_ok_summ',
        'contacts_request_fail_summ',
        'contacts_request_unauthorized_summ',
        'contacts_request_forbidden_summ',

        'smart_home_request_ok_summ',
        'smart_home_request_fail_summ',
        'smart_home_request_unauthorized_summ',
        'smart_home_request_forbidden_summ',

        'memento_get_request_ok_summ',
        'memento_get_request_fail_summ',
        'memento_get_request_forbidden_summ',

        'memento_update_request_ok_summ',
        'memento_update_request_fail_summ',
        'memento_update_request_forbidden_summ',

        'register_device_ok_summ',
        'register_device_fail_summ',
        'unregister_device_ok_summ',
        'unregister_device_fail_summ',

        'tts_fallback_err_summ',

        'tts_cache_post_summ',
        'tts_cache_post_err_summ',
        'tts_cache_post_timeout_summ',
        'tts_cache_get_ok_summ',
        'tts_cache_get_miss_summ',
        'tts_cache_get_timeout_summ',

        'tts_full_precache_ok_summ',
        'tts_full_precache_fail_summ',

        'tts_precache_ok_summ',
        'tts_precache_memcache_miss_summ',
        'tts_precache_miss_summ',

        *tts_model_signals("gpu"),
        *tts_model_signals("gpu_valtz"),
        *tts_model_signals("gpu_oksana"),
        *tts_model_signals("ru"),

        "u2_apply_session_context_summ",
        "u2_asc_nodiff_summ",
        "u2_asc_diff_app_token_summ",
        "u2_asc_diff_oauth_token_summ",
        "u2_asc_diff_uuid_summ",
        "u2_asc_diff_puid_summ",
        "u2_asc_diff_guid_summ",
        "u2_asc_diff_yuid_summ",
        "u2_asc_diff_nologs_summ",
        "u2_asc_diff_uaas_summ",
        "u2_asc_diff_laas_summ",

        'vins_response_sent_summ',
        'vins_processor_events_summ',

        'vins_partial_good_summ',
        'vins_partial_bad_summ',
        'vins_partial_trash_vins_summ',
        'vins_partial_trash_asr_summ',
        'vins_partial_good_wait_summ',

        'yabio_user_matched_summ',

        "u2_context_load_diff_check_summ",
        "u2_cld_nodiff_summ",
        "u2_cld_diff_memento_summ",
        "u2_cld_diff_datasync_summ",
        "u2_cld_diff_datasync_device_id_summ",
        "u2_cld_diff_datasync_uuid_summ",
        "u2_cld_diff_quasar_iot_summ",
        "u2_cld_diff_notificator_summ",
        "u2_cld_diff_megamind_session_summ",

        "u2_count_context_save_summ",
        "u2_failed_context_save_summ",

        "u2_cls_count_mark_notification_as_read_summ",
        "u2_cls_count_personal_cards_summ",
        "u2_cls_count_send_push_directive_summ",
        "u2_cls_count_delete_personal_cards_summ",
        "u2_cls_count_push_message_summ",
        "u2_cls_count_update_datasync_summ",
        "u2_cls_count_update_memento_summ",
        "u2_cls_count_update_notification_subscription_summ",

        "u2_cls_diff_mark_notification_as_read_summ",
        "u2_cls_diff_personal_cards_summ",
        "u2_cls_diff_send_push_directive_summ",
        "u2_cls_diff_delete_personal_cards_summ",
        "u2_cls_diff_push_message_summ",
        "u2_cls_diff_update_datasync_summ",
        "u2_cls_diff_update_memento_summ",
        "u2_cls_diff_update_notification_subscription_summ",

        "u2_count_apphosted_tts_summ",
        "u2_failed_apphosted_tts_summ",

        "u2_count_backward_apphosted_tts_summ",
        "u2_failed_backward_apphosted_tts_summ",

        "u2_system_stream_id_summ",
        "u2_ref_stream_id_summ",

        "cld_apply_personal_data_ok_summ",
        "cld_apply_personal_data_err_summ",
        "cld_apply_memento_ok_summ",
        "cld_apply_memento_err_summ",
        "cld_apply_contacts_ok_summ",
        "cld_apply_contacts_err_summ",
        "cld_apply_notificator_ok_summ",
        "cld_apply_notificator_err_summ",
        "cld_apply_vins_session_ok_summ",
        "cld_apply_vins_session_err_summ",
        "cld_apply_smart_home_ok_summ",
        "cld_apply_smart_home_err_summ",
        "cld_apply_flags_json_ok_summ",
        "cld_apply_flags_json_err_summ",
        "cld_apply_laas_ok_summ",
        "cld_apply_laas_err_summ",
        "cld_apply_cancelled_summ",

        "set_laas_from_cl_to_vins_summ",

        "cachalot_mm_save_ok_summ",
        "cachalot_mm_save_err_summ",
        "cachalot_mm_load_ok_summ",
        "cachalot_mm_load_err_summ",
        "cachalot_mm_load_not_found_summ",

        "extdata_processed_summ",
        "extdata_dropped_summ",

        "s3_audio_ok_summ",
        "s3_audio_err_summ",
        "s3_audio_cache_size_max",

        'matrix_send_push_ok_summ',
        'matrix_send_push_err_summ',
        'matrix_send_push_other_err_summ',
        'matrix_send_push_timeout_summ',

        'matrix_send_sup_push_ok_summ',
        'matrix_send_sup_push_err_summ',
        'matrix_send_sup_push_other_err_summ',
        'matrix_send_sup_push_timeout_summ',

        'matrix_send_sup_card_ok_summ',
        'matrix_send_sup_card_err_summ',
        'matrix_send_sup_card_other_err_summ',
        'matrix_send_sup_card_timeout_summ',

        'matrix_delete_pushes_ok_summ',
        'matrix_delete_pushes_err_summ',
        'matrix_delete_pushes_other_err_summ',
        'matrix_delete_pushes_timeout_summ',

        'matrix_manage_subscription_ok_summ',
        'matrix_manage_subscription_err_summ',
        'matrix_manage_subscription_other_err_summ',
        'matrix_manage_subscription_timeout_summ',

        'matrix_get_state_ok_summ',
        'matrix_get_state_err_summ',
        'matrix_get_state_other_err_summ',
        'matrix_get_state_timeout_summ',

        'matrix_change_status_ok_summ',
        'matrix_change_status_err_summ',
        'matrix_change_status_other_err_summ',
        'matrix_change_status_timeout_summ',

        'matrix_ack_directive_ok_summ',
        'matrix_ack_directive_err_summ',
        'matrix_ack_directive_other_err_summ',
        'matrix_ack_directive_timeout_summ',

        'matrix_register_ok_summ',
        'matrix_register_err_summ',
        'matrix_register_other_err_summ',
        'matrix_register_timeout_summ',

        'matrix_push_typed_semantic_frame_ok_summ',
        'matrix_push_typed_semantic_frame_err_summ',
        'matrix_push_typed_semantic_frame_other_err_summ',
        'matrix_push_typed_semantic_frame_timeout_summ',

        'matrix_add_schedule_action_ok_summ',
        'matrix_add_schedule_action_err_summ',
        'matrix_add_schedule_action_other_err_summ',
        'matrix_add_schedule_action_timeout_summ',
    ]

    _extended_counters = [
        ['asr_5xx_err', 'summ'],
        ['asr_4xx_err', 'summ'],
        ['asr_2xx_ok', 'summ'],
        ['asr_client_timeout_err', 'summ'],
        ['asr_server_timeout_err', 'summ'],
        ['asr_other_err', 'summ'],
        ['asr_cancel_err', 'summ'],

        ['dup_yaldi_5xx_err', 'summ'],
        ['dup_yaldi_4xx_err', 'summ'],
        ['dup_yaldi_2xx_ok', 'summ'],
        ['dup_yaldi_client_timeout_err', 'summ'],
        ['dup_yaldi_server_timeout_err', 'summ'],
        ['dup_yaldi_other_err', 'summ'],
        ['dup_yaldi_cancel_err', 'summ'],

        ['event_exceptions', 'summ'],

        ['spotter_5xx_err', 'summ'],
        ['spotter_4xx_err', 'summ'],
        ['spotter_2xx_ok', 'summ'],
        ['spotter_client_timeout_err', 'summ'],
        ['spotter_server_timeout_err', 'summ'],
        ['spotter_other_err', 'summ'],
        ['spotter_cancel_err', 'summ'],

        ['tts_5xx_err', 'summ'],
        ['tts_4xx_err', 'summ'],
        ['tts_2xx_ok', 'summ'],
        ['tts_other_err', 'summ'],

        ['tts_oksana_5xx_err', 'summ'],
        ['tts_oksana_4xx_err', 'summ'],
        ['tts_oksana_2xx_ok', 'summ'],
        ['tts_oksana_other_err', 'summ'],

        ['vins_5xx_err', 'summ'],
        ['vins_596_err', 'summ'],
        ['vins_512_err', 'summ'],
        ['vins_504_err', 'summ'],
        ['vins_502_err', 'summ'],
        ['vins_500_err', 'summ'],
        ['vins_4xx_err', 'summ'],
        ['vins_2xx_ok', 'summ'],
        ['vins_other_err', 'summ'],

        ['vins_context_too_long', 'summ'],

        ['yabio_5xx_err', 'summ'],
        ['yabio_4xx_err', 'summ'],
        ['yabio_2xx_ok', 'summ'],
        ['yabio_other_err', 'summ'],

        ['yaldi_5xx_err', 'summ'],
        ['yaldi_4xx_err', 'summ'],
        ['yaldi_2xx_ok', 'summ'],
        ['yaldi_client_timeout_err', 'summ'],
        ['yaldi_server_timeout_err', 'summ'],
        ['yaldi_other_err', 'summ'],
        ['yaldi_cancel_err', 'summ'],
    ]

    @classmethod
    def init(cls):
        for name in cls._uniproxy_counters:
            GlobalCounter.register_counter(name)

        GlobalCounter.init_extended(cls._extended_counters)
        GlobalCounter.init()


class UniproxyTimings:

    __uniproxy_counters = [
        *asr_model_timings("dialoggeneral", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        *asr_model_timings("dialoggeneral_gpu", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        *asr_model_timings("desktopgeneral", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        *asr_model_timings("quasargeneral", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        *asr_model_timings("quasargeneral_gpu", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        *asr_model_timings("dialogmaps", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        *asr_model_timings("dialogmaps_gpu", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),

        ('asr_connect_time',            TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_connect_time',            TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('legacy_navi_request_time',    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('yabio_connect_time',          TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('yaldi_connect_time',          TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (VINS_REQUEST_SIZE_QUASAR_HGRAM,  TimingsResolution.REQUEST_SIZE_VALUES),
        (VINS_RESPONSE_SIZE_QUASAR_HGRAM, TimingsResolution.REQUEST_SIZE_VALUES),
        (VINS_REQUEST_SIZE_OTHER_HGRAM,   TimingsResolution.REQUEST_SIZE_VALUES),
        (VINS_RESPONSE_SIZE_OTHER_HGRAM,  TimingsResolution.REQUEST_SIZE_VALUES),
        (VINS_RUN_WAIT_AFTER_EOU_HGRAM,             TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (VINS_RUN_WAIT_AFTER_EOU_HGRAM + "_quasar", TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (BIOMETRY_SCORING_CONFIDENCE_HGRAM,  TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (ASR_MERGE_HGRAM,                    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (ASR_MERGE_FIRST_RESPONSE_HGRAM,     TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (ASR_MERGE_SECOND_RESPONSE_HGRAM,    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (YALDI_PARTIAL_HGRAM,           TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (YABIO_PARTIAL_HGRAM,           TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (CLASSIFY_PARTIAL_HGRAM,        TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (VINS_REQUEST_HGRAM,            TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (TTS_REQUEST_HGRAM,             TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (VINS_LOAD_CONTEXT_HGRAM,       TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (VINS_SAVE_CONTEXT_HGRAM,       TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('datasync_settings_response',  TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (CONTACTS_REQUEST_HGRAM,        TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (SMART_HOME_REQUEST_HGRAM,      TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (MEMENTO_GET_REQUEST_HGRAM,     TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (MEMENTO_UPDATE_REQUEST_HGRAM,  TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('register_device',             TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('unregister_device',           TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (DATASYNC_REQUEST_HGRAM,        TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (BIOMETRY_SCORING_CONFIDENCE_HGRAM,  TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (ASR_MERGE_HGRAM,                    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (ASR_MERGE_FIRST_RESPONSE_HGRAM,     TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (ASR_MERGE_SECOND_RESPONSE_HGRAM,    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('asr_connect_time',            TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_connect_time',            TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('legacy_navi_request_time',    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('yabio_connect_time',          TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('yaldi_connect_time',          TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('offset_yaldi_eou',            TimingsResolution.DEST_COUNTER_VALUES),
        ('spotter_multi_activation',    TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('spotter_multi_activation_final',    TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('spotter_multi_activation_check',    TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('spotter_multi_activation_check_two_steps',    TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('spotter_ahead_asr_time', TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('spotter_ahead_vins_time', TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),

        ('cachalot_mm_save_time', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('cachalot_mm_load_time', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),

        ('tts_gpu_request', TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('tts_gpu_valtz_request', TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('tts_gpu_oksana_request', TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('tts_ru_us_request', TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),

        ('vins_uaas_proc_dur', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),      # duration of VINS' UaaS & experiments

        ('mssngr_location_store_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),

        ('vins_rerequest_quasar_user', TimingsResolution.REREQUEST_DELAY_VALUES),
        ('vins_rerequest_other_user', TimingsResolution.REREQUEST_DELAY_VALUES),

        ('datasync_addresses_get_size_hgram',   TimingsResolution.REQUEST_SIZE_VALUES),
        ('datasync_kv_get_size_hgram',          TimingsResolution.REQUEST_SIZE_VALUES),
        ('datasync_settings_get_size_hgram',    TimingsResolution.REQUEST_SIZE_VALUES),
        ('datasync_addresses_upd_size_hgram',   TimingsResolution.REQUEST_SIZE_VALUES),
        ('datasync_kv_upd_size_hgram',          TimingsResolution.REQUEST_SIZE_VALUES),
        ('datasync_settings_upd_size_hgram',    TimingsResolution.REQUEST_SIZE_VALUES),

        ('push_ack_waiting_hgram', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),

        ('matrix_send_push_time',                 TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_send_sup_push_time',             TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_send_sup_card_time',             TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_delete_pushes_time',             TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_manage_subscription_time',       TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_get_state_time',                 TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_change_status_time',             TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_ack_directive_time',             TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_register_time',                  TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_push_typed_semantic_frame_time', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('matrix_add_schedule_action_time',       TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
    ]

    _quasar_splitted_counters = [  # also exists metrics whith quasar_ prefix
        (TTS_FIRST_CHUNK_HGRAM,             TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (RESPONSE_DELAY_HGRAM,              TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),

        (CLIENT_VINS_RESPONSE_HGRAM,    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (CLIENT_EOU_HGRAM,              TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (CLIENT_FIRST_MERGED_HGRAM,     TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (CLIENT_FIRST_PARTIAL_HGRAM,    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (CLIENT_FIRST_SYNTH_HGRAM,      TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (CLIENT_LAST_PARTIAL_HGRAM,     TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (CLIENT_LAST_SYNTH_HGRAM,       TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),

        (VINS_RUN_DELAY_AFTER_EOU_HGRAM,                TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (VINS_RUN_WAIT_AFTER_EOU_HGRAM,                 TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (events.EventVinsWaitAfterEOUDurationSec.NAME,  TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (VINS_APPLY_REQUEST_DURATION_HGRAM,             TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        (USEFUL_VINS_APPLY_PREPEARE_REQUEST_DURATION,   TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (USEFUL_VINS_APPLY_REQUEST_DURATION_HGRAM,      TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),

        # vins prepare request stage delayers
        (events.EventUsefulVinsPrepareRequestAsr.NAME,          TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestClassify.NAME,     TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestMusic.NAME,        TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestPersonalData.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestSession.NAME,      TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestYabio.NAME,        TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestMemento.NAME,      TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestNotificationState.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestContacts.NAME,     TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulVinsPrepareRequestLaas.NAME,         TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),

        # timings independent from user phrase lengths (audio) for VOICESERV-3170
        (events.EventUsefulVinsRequestDuration.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('user_useful_partial_to_asr_end', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('robot_useful_partial_to_asr_end', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
    ]
    _quasar_splitted2_counters = [  # also exists metrics whith quasar_ prefix
        # times from event's birth
        (events.EventAsrContextsReady.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventVinsPersonalDataStart.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventVinsPersonalDataEnd.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventVinsSessionLoadEnd.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventVinsResponseSent.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventTtsStart.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (events.EventUsefulResponseForUser.NAME, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_start_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_uaas_start_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('user_vins_uaas_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('robot_vins_uaas_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_response_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_request_eou_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_apply_request_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_session_save_start_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_session_save_end_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_update_user_start_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_remove_user_start_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('spotter_start_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('spotter_end_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('user_spotter_validation_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('robot_spotter_validation_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('scoring_start_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('scoring_end_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('asr_start_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('asr_first_chunk_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('asr_end_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('useful_asr_result_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_first_chunk_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_first_chunk_nocache_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('yabio_end_evage', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_response_to_tts_first_chunk_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_response_to_tts_first_chunk_user_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_response_sent_to_tts_first_chunk_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_response_sent_to_tts_first_chunk_user_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('asr_end_to_vins_response_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('user_asr_end_to_vins_response_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('robot_asr_end_to_vins_response_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_request_eou_to_vins_response_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('execute_vins_directives_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('user_execute_vins_directives_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('robot_execute_vins_directives_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('vins_session_save_duration', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('user_vins_session_save_duration', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('robot_vins_session_save_duration', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('user_tts_cache_response_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('robot_tts_cache_response_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_start_to_tts_first_chunk_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_start_to_tts_first_chunk_user_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_start_to_tts_first_chunk_robot_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_start_to_tts_first_chunk_nocache_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_start_to_tts_first_chunk_user_nocache_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('asr_end_to_useful_response_for_user_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('asr_end_to_useful_response_for_robot_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('useful_asr_result_to_vins_personal_data_end_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('useful_asr_result_to_useful_response_for_user_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('useful_asr_result_to_useful_response_for_robot_lag', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
    ]

    @classmethod
    def init(cls):
        for name, boundaries in cls.__uniproxy_counters:
            GlobalTimings.register_counter(name, boundaries)
        for name, boundaries in cls._quasar_splitted_counters:
            GlobalTimings.register_counter(name, boundaries)
            GlobalTimings.register_counter(name + '_quasar', boundaries)
        for name, boundaries in cls._quasar_splitted2_counters:
            GlobalTimings.register_counter(name, boundaries)
            GlobalTimings.register_counter('quasar_' + name, boundaries)
        GlobalTimings.init()


class UniproxyGolovanBackend:
    __instance__ = None

    def __init__(self):
        pass

    def rate(self, name, count=1):
        UniproxyCounter.increment_by_name(name, count)

    def hgram(self, name, duration):
        UniproxyTimings.store(name, duration)

    @staticmethod
    def instance():
        if UniproxyGolovanBackend.__instance__ is None:
            UniproxyGolovanBackend.__instance__ = UniproxyGolovanBackend()
        return UniproxyGolovanBackend.__instance__
