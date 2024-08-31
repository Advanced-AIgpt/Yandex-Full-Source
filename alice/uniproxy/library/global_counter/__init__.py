import ctypes
import itertools
import json
import multiprocessing
import time

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config

EC_CLIENT_INACTIVITY_TIMEOUT = 608
EC_SERVER_INACTIVITY_TIMEOUT = 688
EC_CANCEL = 609

OTHER_DEVICES = 'other_devices'

PERSONAL_CARDS_REQUEST_ADD_HGRAM = 'pesonal_cards_add'
PERSONAL_CARDS_REQUEST_DISMISS_HGRAM = 'pesonal_cards_dismiss'
PERSONAL_CARDS_REQUEST_GET_HGRAM = 'pesonal_cards_get'


class TimingsResolution:
    ULTRA_RESOLUTION_COUNTER_VALUES = (
        0.002, 0.004, 0.006, 0.008, 0.010, 0.012, 0.014, 0.016, 0.018, 0.020, 0.025, 0.030, 0.035, 0.040,
        0.050, 0.060, 0.070, 0.080, 0.090, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
        2.0, 3.0, 5.0, 7.5, 10.0
    )

    HIGH_RESOLUTION_COUNTER_VALUES = (
        0.005, 0.010, 0.015, 0.020, 0.025, 0.030, 0.035, 0.040, 0.045, 0.050, 0.06, 0.07, 0.08, 0.09,
        0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 2.0, 3.0, 5.0, 7.5, 10.0
    )

    LOW_RESOLUTION_COUNTER_VALUES = (
        0.0, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 5.0, 7.5, 10.0
    )

    QUEUE_COUNTER_VALUES = (
        50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 600, 700, 800, 900, 1000, 1200, 1400, 1600, 1800, 2000,
        2200, 2400, 2600, 2800, 3000, 3500, 4000, 4500, 5000
    )

    DEST_COUNTER_VALUES = (
        5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 125, 150, 175, 200, 250, 300, 350, 400, 450, 500, 600, 700,
        800, 900, 1000, 1200, 1400, 1600, 1800, 2000, 2500, 3000, 3500, 4000, 4500
    )

    REQUEST_SIZE_VALUES = (
        100, 200, 300, 400, 500, 750, 1500, 3000, 5000, 10000, 50000, 100000, 500000, 1000000, 2000000
    )

    AUDIO_SIZE_VALUES = (  # more appropriate for audio encoded as PCM16/8KHz
        640, 1600, 3200, 6400, 12800, 25600, 32000, 48000, 64000, 96000, 128000, 160000, 224000, 320000, 480000
    )

    REREQUEST_DELAY_VALUES = (
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 20, 30, 40, 50, 60, 70, 80, 90, 100,
    )


# ====================================================================================================================
class GlobalTimingVariable(object):
    def __init__(self, boundaries):
        self._boundaries = boundaries
        self._size = len(self._boundaries)
        self._value = multiprocessing.Array(ctypes.c_longlong, self._size, lock=True)

    # ----------------------------------------------------------------------------------------------------------------
    def reset(self):  # for tests
        with self._value.get_lock():
            for i in range(0, self._size):
                self._value[i] = 0

    # ----------------------------------------------------------------------------------------------------------------
    def store(self, value):
        index = 0

        if value > 0.0:
            index = 1
            while index < self._size and value > self._boundaries[index]:
                index += 1
            index = index - 1

        with self._value.get_lock():
            self._value[index] += 1

    # ----------------------------------------------------------------------------------------------------------------
    def to_list(self):
        with self._value.get_lock():
            data = list(self._value)
        return list(zip(self._boundaries, data))


class GlobalTimings(object):
    __g_timings = {}

    __g_inited = False

    __counters = [  # common counters
        ('mssngr_auth_wait',            TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('fanout_auth_wait',            TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('mssngr_upd_location',         TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('mssngr_del_location',         TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('mssngr_in_t_wait',            TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('mssngr_in_q_wait',            TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('mssngr_in_d_wait',            TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('mssngr_out_resolve',          TimingsResolution.ULTRA_RESOLUTION_COUNTER_VALUES),
        ('mssngr_out_post',             TimingsResolution.ULTRA_RESOLUTION_COUNTER_VALUES),
        ('mssngr_out_total',            TimingsResolution.ULTRA_RESOLUTION_COUNTER_VALUES),
        ('mssngr_history_time',         TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('mssngr_subscribe_time',       TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('mssngr_whoami_time',          TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('mssngr_minfo_time',           TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('mssngr_edit_history_time',    TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('subway_delivery_time',        TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('memcached_set_time',          TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('memcached_get_time',          TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('tts_cache_get_time',          TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('tts_cache_post_time',         TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('uniprx_ev_hndl_time',         TimingsResolution.LOW_RESOLUTION_COUNTER_VALUES),
        ('ydb_prepare_req',             TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('unimsg_out_total',            TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('unimsg_out_resolve',          TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('unimsg_out_post',             TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('synchronize_state_wait',      TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('subway_queue_size',           TimingsResolution.QUEUE_COUNTER_VALUES),
        ('subway_dest_count',           TimingsResolution.DEST_COUNTER_VALUES),
        ('mssngr_auth_fanout_blackbox_time', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        ('mssngr_auth_yamb_blackbox_time',   TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),

        ('mssngr_location_store_total', TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),

        (PERSONAL_CARDS_REQUEST_ADD_HGRAM,     TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (PERSONAL_CARDS_REQUEST_DISMISS_HGRAM, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
        (PERSONAL_CARDS_REQUEST_GET_HGRAM, TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES),
    ]

    # ----------------------------------------------------------------------------------------------------------------
    @classmethod
    def init(cls):
        for name, boundaries in cls.__counters:
            cls.__g_timings[name] = GlobalTimingVariable(boundaries)
        cls.__g_inited = True

    # ----------------------------------------------------------------------------------------------------------------
    @classmethod
    def register_counter(cls, name, boundaries=TimingsResolution.HIGH_RESOLUTION_COUNTER_VALUES):
        cls.__counters.append((name, boundaries))
        if not cls.__g_timings:
            Logger.get('.global_counter').debug('GlobalTimings is not inited, just add to list')
            return
        cls.__g_timings[name] = GlobalTimingVariable(boundaries)

    @classmethod
    def register_counters(cls, *args):
        for i in args:
            if isinstance(i, tuple):
                cls.register_counter(i[0], i[1])
            else:
                cls.register_counter(i)

    # ----------------------------------------------------------------------------------------------------------------
    @staticmethod
    def reset(name):  # for tests
        variable = GlobalTimings.__g_timings.get(name)
        if variable is not None:
            variable.reset()

    # ----------------------------------------------------------------------------------------------------------------
    @staticmethod
    def store(name, timing):
        if timing < 0.0:
            return

        variable = GlobalTimings.__g_timings.get(name)
        if variable is None:
            return

        variable.store(timing)

    # ----------------------------------------------------------------------------------------------------------------
    @staticmethod
    def get_metrics():
        return [
            [
                '{}_hgram'.format(name),
                value.to_list()
            ] for name, value in GlobalTimings.__g_timings.items()
        ]


# ====================================================================================================================
class UnistatTiming(object):
    @classmethod
    def wrapped(cls, name):

        def decorator(func):
            import asyncio
            import functools

            if asyncio.iscoroutinefunction(func):
                @functools.wraps(func)
                async def wrap(*args, **kwargs):
                    with cls(name):
                        return await func(*args, **kwargs)
            else:
                @functools.wraps(func)
                def wrap(*args, **kwargs):
                    with cls(name):
                        return func(*args, **kwargs)
            return wrap

        return decorator

    def __init__(self, name, logger=None):
        self._counter = name
        self._initial_time = 0.0
        self._logger = logger
        self.duration = None

    @property
    def start_ts(self):
        return self._initial_time

    def __enter__(self):
        return self.start()

    def __exit__(self, *args, **kwargs):
        self.stop()

    def start(self):
        self._initial_time = time.time()
        return self

    def stop(self):
        self.duration = time.time() - self._initial_time
        GlobalTimings.store(self._counter, self.duration)
        if self._logger:
            self._logger.debug('UnistatTiming(%s) took %.3fms' % (self._counter, self.duration * 1000))
            self._logger = None


# ====================================================================================================================
class GlobalCounter(object):
    g_counters = {}
    g_inited = False

    _counters = [  # it's common. for uniproxy see below
        'handler_asrdemo_html_reqs_summ',
        'handler_asrsocket_ws_reqs_summ',
        'handler_common_reqs_summ',
        'handler_convertercheck_reqs_summ',
        'handler_delivery_reqs_summ',
        'handler_envcheck_reqs_summ',
        'handler_exp_reqs_summ',
        'handler_huni_ws_reqs_summ',
        'handler_legacy_toyota_reqs_summ',
        'handler_logsocket_ws_reqs_summ',
        'handler_memcheck_reqs_summ',
        'handler_monitoring_reqs_summ',
        'handler_ping_reqs_summ',
        'handler_revision_reqs_summ',
        'handler_settings_js_reqs_summ',
        'handler_speakers_reqs_summ',
        'handler_start_hook_reqs_summ',
        'handler_stop_hook_reqs_summ',
        'handler_textinputcheck_reqs_summ',
        'handler_tts_generate_reqs_summ',
        'handler_ttsdemo_html_reqs_summ',
        'handler_ttssocket_ws_reqs_summ',
        'handler_uni_ws_reqs_summ',
        'handler_unistat_reqs_summ',
        'handler_unknown_reqs_summ',
        'handler_voiceinputcheck_reqs_summ',

        'vins_e2e_partial_summ',
        'vins_yaldi_partial_summ',

        'vins_full_result_ready_on_eou_summ',
        'vins_full_result_not_ready_on_eou_summ',

        'event_reply_timedout_summ',

        'memcached_sets_summ',
        'memcached_gets_summ',
        'memcached_set_errors_summ',
        'memcached_get_errors_summ',

        'blackbox_5xx_err_summ',
        'blackbox_4xx_err_summ',
        'blackbox_2xx_ok_summ',
        'blackbox_other_err_summ',

        'laas_5xx_err_summ',
        'laas_4xx_err_summ',
        'laas_2xx_ok_summ',
        'laas_other_err_summ',

        'flags_json_2xx_ok_summ',
        'flags_json_4xx_err_summ',
        'flags_json_5xx_err_summ',
        'flags_json_other_err_summ',
        'flags_json_bad_response_data_summ',

        'flags_json_context_load_empty_summ',
        'flags_json_context_load_nonempty_summ',
        'flags_json_context_load_total_count_summ',
        'flags_json_invalid_call_source_empty_summ',
        'flags_json_invalid_call_source_nonempty_summ',
        'flags_json_invalid_call_source_total_count_summ',
        'flags_json_set_state_empty_summ',
        'flags_json_set_state_nonempty_summ',
        'flags_json_set_state_total_count_summ',
        'flags_json_sync_state_empty_summ',
        'flags_json_sync_state_nonempty_summ',
        'flags_json_sync_state_total_count_summ',
        'flags_json_zalipanie_empty_summ',
        'flags_json_zalipanie_nonempty_summ',
        'flags_json_zalipanie_total_count_summ',

        'apikeys_5xx_err_summ',
        'apikeys_4xx_err_summ',
        'apikeys_2xx_ok_summ',
        'apikeys_other_err_summ',

        'ws_tts_5xx_err_summ',
        'ws_tts_4xx_err_summ',
        'ws_tts_2xx_ok_summ',
        'ws_tts_other_err_summ',

        'ws_asr_5xx_err_summ',
        'ws_asr_4xx_err_summ',
        'ws_asr_2xx_ok_summ',
        'ws_asr_other_err_summ',

        'mds_save_with_tvm_summ',
        'mds_save_without_tvm_summ',
        'mds_written_bytes_summ',
        'mds_5xx_err_summ',
        'mds_4xx_err_summ',
        'mds_2xx_ok_summ',
        'mds_other_err_summ',

        "exp_oksana.gpu_to_good_oksana_summ",

        'sd_2xx_ok_summ',
        'sd_4xx_err_summ',
        'sd_5xx_err_summ',
        'sd_598_err_summ',
        'sd_599_err_summ',

        'uniprx_ws_open_summ',
        'uniprx_ws_close_summ',
        'uniprx_ws_no_active_summ',
        'uniprx_ws_long_active_summ',
        'uniprx_ws_message_summ',
        'uniprx_ws_current_ammx',
        'uniprx_ev_created_summ',
        'uniprx_ev_closed_summ',
        'uniprx_ev_running_ammx',

        #
        #   MSSNGR AUTH STATUS
        #
        'mssngr_oauth_ok_summ',
        'mssngr_oauth_fail_summ',
        'mssngr_yambauth_ok_summ',
        'mssngr_yambauth_fail_summ',
        'mssngr_oauthteam_ok_summ',
        'mssngr_oauthteam_fail_summ',
        'mssngr_unexpected_auth_summ',

        'mssngr_auth_undefined_ok',
        'mssngr_auth_undefined_fail_summ',
        'mssngr_auth_cookie_ok_summ',
        'mssngr_auth_cookie_fail_summ',
        'mssngr_auth_yambcookie_ok_summ',
        'mssngr_auth_yambcookie_fail_summ',

        #
        #   legacy navi
        #
        'legacy_navi_requests_in_summ',
        'legacy_navi_requests_ok_summ',
        'legacy_navi_requests_err_summ',

        #
        #   MSSNGR AUTH FANOUT
        #
        'mssngr_auth_fanout_200_summ',
        'mssngr_auth_fanout_401_summ',
        'mssngr_auth_fanout_403_summ',
        'mssngr_auth_fanout_4xx_summ',
        'mssngr_auth_fanout_5xx_summ',
        'mssngr_auth_fanout_597_summ',
        'mssngr_auth_fanout_598_summ',
        'mssngr_auth_fanout_599_summ',
        'mssngr_auth_fanout_bb_4xx_summ',
        'mssngr_auth_fanout_bb_5xx_summ',
        'mssngr_auth_fanout_bb_5xx_summ',
        'mssngr_auth_fanout_fail_summ',
        'mssngr_auth_fanout_internal_fail_summ',

        #
        #   MSSNGR AUTH YAMB
        #
        'mssngr_auth_yamb_200_summ',
        'mssngr_auth_yamb_401_summ',
        'mssngr_auth_yamb_403_summ',
        'mssngr_auth_yamb_4xx_summ',
        'mssngr_auth_yamb_5xx_summ',
        'mssngr_auth_yamb_597_summ',
        'mssngr_auth_yamb_598_summ',
        'mssngr_auth_yamb_599_summ',
        'mssngr_auth_yamb_bb_4xx_summ',
        'mssngr_auth_yamb_bb_5xx_summ',
        'mssngr_auth_yamb_bb_5xx_summ',
        'mssngr_auth_yamb_fail_summ',
        'mssngr_auth_yamb_internal_fail_summ',

        'mssngr_messages_in_recv_summ',
        'mssngr_messages_in_recv_plain_summ',
        'mssngr_messages_in_recv_server_summ',
        'mssngr_messages_in_recv_typing_summ',
        'mssngr_messages_in_recv_other_summ',
        'mssngr_messages_in_push_summ',
        'mssngr_messages_in_fail_summ',
        'mssngr_messages_in_fanout_200_summ',
        'mssngr_messages_in_fanout_4xx_summ',
        'mssngr_messages_in_fanout_5xx_summ',
        'mssngr_messages_in_fanout_597_summ',
        'mssngr_messages_in_fanout_598_summ',
        'mssngr_messages_in_fanout_599_summ',
        'mssngr_messages_in_fanout_other_summ',
        'mssngr_messages_in_fanout_rst_summ',
        'mssngr_messages_in_unauthorized_summ',


        'mssngr_voice_messages_in_recv_summ',
        'mssngr_voice_messages_in_push_summ',
        'mssngr_voice_messages_in_err_summ',
        'mssngr_voice_messages_in_fail_summ',
        'mssngr_voice_messages_in_unauthorized_summ',

        'mssngr_set_voice_chats_in_recv_summ',
        'mssngr_set_voice_chats_in_unauthorized_summ',

        'mssngr_history_in_recv_summ',
        'mssngr_history_in_fail_summ',
        'mssngr_history_in_got_summ',
        'mssngr_history_in_fanout_200_summ',
        'mssngr_history_in_fanout_4xx_summ',
        'mssngr_history_in_fanout_5xx_summ',
        'mssngr_history_in_fanout_597_summ',
        'mssngr_history_in_fanout_598_summ',
        'mssngr_history_in_fanout_599_summ',
        'mssngr_history_in_fanout_other_summ',
        'mssngr_history_in_unauthorized_summ',

        'mssngr_edit_history_recv_summ',
        'mssngr_edit_history_fail_summ',
        'mssngr_edit_history_got_summ',
        'mssngr_edit_history_fanout_200_summ',
        'mssngr_edit_history_fanout_4xx_summ',
        'mssngr_edit_history_fanout_5xx_summ',
        'mssngr_edit_history_fanout_597_summ',
        'mssngr_edit_history_fanout_598_summ',
        'mssngr_edit_history_fanout_599_summ',
        'mssngr_edit_history_fanout_other_summ',
        'mssngr_edit_history_unauthorized_summ',

        'mssngr_subscription_in_recv_summ',
        'mssngr_subscription_in_fail_summ',
        'mssngr_subscription_in_got_summ',
        'mssngr_subscription_in_fanout_200_summ',
        'mssngr_subscription_in_fanout_4xx_summ',
        'mssngr_subscription_in_fanout_5xx_summ',
        'mssngr_subscription_in_fanout_597_summ',
        'mssngr_subscription_in_fanout_598_summ',
        'mssngr_subscription_in_fanout_599_summ',
        'mssngr_subscription_in_fanout_other_summ',
        'mssngr_subscription_in_unauthorized_summ',

        'mssngr_whoami_in_recv_summ',
        'mssngr_whoami_in_fail_summ',
        'mssngr_whoami_in_got_summ',
        'mssngr_whoami_in_fanout_200_summ',
        'mssngr_whoami_in_fanout_4xx_summ',
        'mssngr_whoami_in_fanout_5xx_summ',
        'mssngr_whoami_in_fanout_597_summ',
        'mssngr_whoami_in_fanout_598_summ',
        'mssngr_whoami_in_fanout_599_summ',
        'mssngr_whoami_in_fanout_other_summ',
        'mssngr_whoami_in_unauthorized_summ',

        'mssngr_minfo_in_recv_summ',
        'mssngr_minfo_in_fail_summ',
        'mssngr_minfo_in_got_summ',
        'mssngr_minfo_in_fanout_200_summ',
        'mssngr_minfo_in_fanout_4xx_summ',
        'mssngr_minfo_in_fanout_5xx_summ',
        'mssngr_minfo_in_fanout_597_summ',
        'mssngr_minfo_in_fanout_598_summ',
        'mssngr_minfo_in_fanout_599_summ',
        'mssngr_minfo_in_fanout_other_summ',
        'mssngr_minfo_in_unauthorized_summ',

        'mssngr_messages_out_recv_summ',
        'mssngr_messages_out_sent_summ',

        'delivery_tvm_checksrv_5xx_err_summ',
        'delivery_tvm_checksrv_4xx_err_summ',
        'delivery_tvm_checksrv_2xx_ok_summ',
        'delivery_tvm_checksrv_other_err_summ',
        'delivery_location_remove_summ',
        'delivery_location_removed_summ',

        #
        #   SUBWAY RESPONSES
        #
        'delivery_push_200_summ',
        'delivery_push_4xx_summ',
        'delivery_push_5xx_summ',
        'delivery_push_597_summ',
        'delivery_push_598_summ',
        'delivery_push_599_summ',
        'delivery_push_other_err_summ',
        'delivery_push_missing_count_summ',

        'mssngr_messages_out_service_ticket_validation_error_summ',
        'mssngr_messages_out_no_service_ticket_error_summ',
        'mssngr_messages_out_no_location_summ',
        'mssngr_messages_out_invalid_checksum_summ',
        'mssngr_messages_out_invalid_version_summ',
        'mssngr_messages_out_invalid_message_summ',
        'mssngr_messages_out_delivery_error_summ',
        'mssngr_messages_out_fail_summ',

        'mssngr_multi_guid_eq_1_summ',
        'mssngr_multi_guid_gt_1_le_3_summ',
        'mssngr_multi_guid_gt_3_le_5_summ',
        'mssngr_multi_guid_gt_5_le_9_summ',
        'mssngr_multi_guid_gt_9_summ',
        'mssngr_guids_count_summ',

        'mssngr_subway_recv_summ',
        'mssngr_subway_sent_summ',
        'mssngr_subway_not_found_summ',
        'mssngr_subway_invalid_request_summ',
        'mssngr_subway_invalid_transfer_summ',
        'mssngr_subway_connection_dropped_summ',
        'mssngr_subway_timeout_summ',
        'mssngr_subway_unisystem_fail_summ',
        'mssngr_subway_pull_fail_summ',
        'mssngr_subway_fail_summ',

        'music2_request_summ',
        'music2_response_timeout_summ',
        'music2_other_err_summ',
        'music2_classify_not_music_summ',
        'music2_force_finish_summ',
        'music2_classify_music_summ',
        'music2_no_matches_summ',
        'music2_success_summ',

        'vins_partial_200_summ',
        'vins_partial_202_summ',
        'vins_partial_205_summ',

        'spotter_ok_summ',
        'spotter_err_summ',

        'spotter_validation_ok_summ',
        'spotter_validation_fail_summ',
        'spotter_validation_err_summ',
        'spotter_validation_timeout_summ',

        'spotter_activation_requests_summ',
        'spotter_activation_allowed_summ',
        'spotter_activation_cancelled_summ',
        'spotter_activation_invalid_data_summ',
        'spotter_activation_op_timeout_summ',
        'spotter_activation_op_error_summ',

        'spotter_multi_activation_activate_summ',
        'spotter_multi_activation_cancel_summ',
        'spotter_multi_activation_activate_two_steps_summ',
        'spotter_multi_activation_cancel_first_step_summ',
        'spotter_multi_activation_cancel_second_step_summ',
        'spotter_multi_activation_cancel_by_faster_summ',

        # YdbSessionPool errors
        'yabio_storage_conflicts_summ',
        'yabio_registered_users_summ',

        'datasync_request_ok_summ',
        'datasync_request_fail_summ',

        'rtlog_active_loggers_ammm',
        'rtlog_events_summ',
        'rtlog_pending_bytes_ammm',
        'rtlog_written_frames_summ',
        'rtlog_written_bytes_summ',
        'rtlog_errors_summ',
        'rtlog_shrinked_bytes_summ',

        'async_file_logger_messages_count_summ',
        'async_file_logger_errors_count_summ',
        'async_file_logger_dropped_messages_count_summ',

        'mssngr_clients_ammx',
        'mssngr_fake_clients_ammx',
        'mssngr_anon_clients_ammx',
        'mssngr_running_push_ammx',

        'galdi_request_summ',
        'galdi_ok_summ',
        'galdi_error_summ',
        'galdi_client_ok_summ',
        'galdi_client_timeout_summ',
        'galdi_server_timeout_summ',

        'personal_cards_request_add_ok_summ',
        'personal_cards_request_add_fail_summ',
        'personal_cards_request_dismiss_ok_summ',
        'personal_cards_request_dismiss_fail_summ',
        'personal_cards_request_get_ok_summ',
        'personal_cards_request_get_fail_summ',

        'mssngr_location_store_memcached_summ',
        'mssngr_location_store_fail_memcached_summ',
        'mssngr_location_remove_memcached_summ',
        'mssngr_location_remove_batch_size_memcached_summ',


        'contact_list_size_bytes_hgram'
    ]

    _app_types = config.get('vins', {}).get('app_types', {})

    _models = list(config.get('vins', {}).get('device_models', ()))

    def __init__(self, name, ctype="i", initval=0):
        self.val = multiprocessing.Value(ctype, initval)
        self._local_val = initval
        GlobalCounter.g_counters[name] = self

    @classmethod
    def init(cls):
        for name in cls._counters:
            setattr(cls, name.upper(), cls(name))

        cls.g_inited = True

    @classmethod
    def init_extended(cls, counters):
        if OTHER_DEVICES not in cls._models:
            cls._models.append(OTHER_DEVICES)

        for name_parts in counters:
            name = '_'.join(name_parts)
            setattr(GlobalCounter, name.upper(), cls(name))

        if GlobalCounter._app_types:
            for prefix, app_type, suffix in [
                (prefix, app_type, suffix)
                for prefix, suffix in counters
                for app_type in set(cls._app_types.values())
            ]:
                full_name = '_'.join((prefix, app_type, suffix))
                setattr(cls, full_name.upper(), cls(full_name))

            if 'other_apps' not in cls._app_types.values():
                for prefix, suffix in counters:
                    full_name = '_'.join((prefix, 'other_apps', suffix))
                    setattr(cls, full_name.upper(), cls(full_name))

        if GlobalCounter._models:
            for prefix, device_model, suffix in [
                (prefix, device_model, suffix)
                for prefix, suffix in counters
                for device_model in cls._models
            ]:
                full_name = '_'.join((prefix, device_model, suffix))
                setattr(cls, full_name.upper(), cls(full_name))

    @classmethod
    def register_counter(cls, *args):
        for name in args:
            cls._counters.append(name)
            if not cls.g_inited:
                Logger.get('.global_counter').debug('global_counter is not inited, just add to list')
                continue
            setattr(cls, name.upper(), cls(name))

    def increment(self, d=1):
        with self.val.get_lock():
            self.val.value += d
        self._local_val += d

    def decrement(self, d=1):
        with self.val.get_lock():
            self.val.value -= d
        self._local_val -= d

    def set(self, value):
        if value == self._local_val:
            return
        with self.val.get_lock():
            self.val.value += value - self._local_val
        self._local_val = value

    def value(self):
        with self.val.get_lock():
            return self.val.value

    @classmethod
    def get_metrics(cls):
        return [[key, val.value()] for key, val in cls.g_counters.items()]

    @classmethod
    def increment_error_code(cls, service, code, *parts):
        error_name = "other_err"
        if 200 <= code < 300:
            error_name = "2xx_ok"
        elif 400 <= code < 500:
            error_name = "4xx_err"
        elif 500 <= code < 599:
            error_name = "5xx_err"

        if service == 'vins' and error_name == "5xx_err":
            if hasattr(GlobalCounter, f"VINS_{code}_ERR_SUMM"):
                error_name = "{}_err".format(code)

        if service == 'yaldi' or service == 'dup_yaldi' or service == 'asr' or service == 'spotter':
            if code == EC_CLIENT_INACTIVITY_TIMEOUT:
                error_name = 'client_timeout_err'
            elif code == EC_SERVER_INACTIVITY_TIMEOUT:
                error_name = 'server_timeout_err'
            elif code == EC_CANCEL:
                error_name = 'cancel_err'

        counter_name = '_'.join(itertools.chain.from_iterable([[service, error_name], parts or [], ['SUMM']])).upper()
        counter = getattr(GlobalCounter, counter_name, None)
        if counter is not None:
            counter.increment()
        elif not parts:
            Logger.get('.global_counter').warning("Bad counter name: %s", counter_name)

    @classmethod
    def increment_counter(cls, *parts, d=1):
        counter = '_'.join(itertools.chain.from_iterable([parts or [], ['SUMM']])).upper()
        if hasattr(GlobalCounter, counter):
            getattr(GlobalCounter, counter).increment(d)
        else:
            Logger.get('.global_counter').debug("Bad counter name: %s", counter)

    @classmethod
    def increment_by_name(cls, counter, value=1):
        if hasattr(GlobalCounter, counter):
            getattr(GlobalCounter, counter).increment(value)


class GlobalCountersUpdater:
    _callbacks = {}
    _cookie = 0

    @classmethod
    def register(cls, callback):
        cls._cookie += 1
        cls._callbacks[cls._cookie] = callback
        return cls._cookie

    @classmethod
    def unregister(cls, cookie):
        del cls._callbacks[cookie]

    @classmethod
    def update(cls):
        try:
            for c in cls._callbacks.values():
                c()
        except Exception as exc:
            Logger.get('.global_counter').exception(exc)


class Unistat:
    _robot_prefixes = (
        'aaaaaaaaaaaaaaaa',
        'bbbbbbbbbbbbbbbb',
        'cccccccccccccccc',
        'dddddddddddddddd',
        'deadbeef',
        'feedbeef',
        'feedface',
        'ffffffff',
    )

    def __init__(self, unisystem):
        self.unisystem = unisystem  # ALARM: weakref proxy!
        self.is_quasar = False
        self.is_robot = False

    def check_uuid(self, uuid):
        uuid = uuid.replace('-', '')
        for prefix in self._robot_prefixes:
            if uuid.startswith(prefix):
                self.is_robot = True
                break

    def _extended_metric_name(self, metric_name, quasar_as_suffix=True):
        try:
            if self.unisystem.closed:
                return  # not increment metrics for zombi events
        except ReferenceError:
            return  # not increment metrics for zombi session

        user_metric = 'useful' in metric_name or 'user' in metric_name
        robot_metric = 'robot' in metric_name
        if robot_metric:
            user_metric = False  # support *_useful_*_robot_* metrics
        if self.is_robot:
            if user_metric:
                return  # not increment users metrics for robot requests
        else:  # alive user
            if robot_metric:
                return  # not increment robot metrics for user requests

        if self.is_quasar:
            if quasar_as_suffix:
                metric_name += '_quasar'
            else:
                metric_name = 'quasar_' + metric_name
        return metric_name

    def store(self, metric_name, value):
        """add _quasar suffix if need"""
        metric_name = self._extended_metric_name(metric_name, quasar_as_suffix=True)
        if not metric_name:
            return

        GlobalTimings.store(metric_name, value)

    def store2(self, metric_name, value):
        """add quasar_ prefix if need"""
        metric_name = self._extended_metric_name(metric_name, quasar_as_suffix=False)
        if not metric_name:
            return

        GlobalTimings.store(metric_name, value)

    def inc_counter2(self, metric_name):
        """add quasar_ prefix if need"""
        metric_name = self._extended_metric_name(metric_name, quasar_as_suffix=False)
        if not metric_name:
            return

        GlobalCounter.increment_counter(metric_name)


class GolovanBackend:
    __instance__ = None

    def __init__(self):
        pass

    def rate(self, name, count=1):
        GlobalCounter.increment_by_name(name, count)

    def hgram(self, name, duration):
        GlobalTimings.store(name, duration)

    @staticmethod
    def instance():
        if GolovanBackend.__instance__ is None:
            GolovanBackend.__instance__ = GolovanBackend()
        return GolovanBackend.__instance__


# ====================================================================================================================
if __name__ == '__main__':
    GlobalTimings.init()
    GlobalTimings.store('mssngr_in_q_wait', 0.44)
    print(json.dumps(GlobalTimings.get_metrics(), indent=4))
