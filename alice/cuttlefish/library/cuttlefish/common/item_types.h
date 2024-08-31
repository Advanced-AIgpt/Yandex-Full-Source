#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NCuttlefish {

// protobuf NAliceProtocol::TSessionContext (alice/cuttlefish/library/protos/session.proto)
constexpr TStringBuf ITEM_TYPE_SESSION_CONTEXT = "session_context";

// protobuf NAliceProtocol::TRequestContext (alice/cuttlefish/library/protos/session.proto)
constexpr TStringBuf ITEM_TYPE_REQUEST_CONTEXT = "request_context";

// protobuf NAliceProtocol::TSynchronizeStateEvent (alice/cuttlefish/library/protos/events.proto)
constexpr TStringBuf ITEM_TYPE_SYNCRHONIZE_STATE_EVENT = "synchonize_state_event";

// protobuf NAliceProtocol::TDirective/Exception (alice/cuttlefish/library/protos/events.proto)
constexpr TStringBuf ITEM_TYPE_EVENT_EXCEPTION = "event_exception";
constexpr TStringBuf ITEM_TYPE_DIRECTIVE = "directive";

// protobuf NAliceProtocol::TWsEvent (alice/cuttlefish/library/protos/wsevent.proto)
constexpr TStringBuf ITEM_TYPE_WS_MESSAGE = "ws_message";

constexpr TStringBuf ITEM_TYPE_SETTINGS = "settings_from_manager";
constexpr TStringBuf ITEM_TYPE_ALICE_LOGGER_OPTIONS = "alice_logger_options";
constexpr TStringBuf ITEM_TYPE_LOGGER_OPTIONS = "logger_options";

constexpr TStringBuf ITEM_TYPE_APIKEYS_HTTP_REQUEST = "apikeys_http_request";
constexpr TStringBuf ITEM_TYPE_APIKEYS_HTTP_RESPONSE = "apikeys_http_response";

constexpr TStringBuf ITEM_TYPE_BLACKBOX_HTTP_REQUEST = "blackbox_http_request";
constexpr TStringBuf ITEM_TYPE_BLACKBOX_HTTP_RESPONSE = "blackbox_http_response";

constexpr TStringBuf ITEM_TYPE_GUEST_BLACKBOX_HTTP_REQUEST = "guest_blackbox_http_request";
constexpr TStringBuf ITEM_TYPE_GUEST_BLACKBOX_HTTP_RESPONSE = "guest_blackbox_http_response";

constexpr TStringBuf ITEM_TYPE_DATASYNC_HTTP_REQUEST = "datasync_http_request";
constexpr TStringBuf ITEM_TYPE_DATASYNC_HTTP_RESPONSE = "datasync_http_response";

constexpr TStringBuf ITEM_TYPE_GUEST_DATASYNC_HTTP_REQUEST = "guest_datasync_http_request";
constexpr TStringBuf ITEM_TYPE_GUEST_DATASYNC_HTTP_RESPONSE = "guest_datasync_http_response";

constexpr TStringBuf ITEM_TYPE_DATASYNC_CACHE_GET_REQUEST = "datasync_cache_get_request";
constexpr TStringBuf ITEM_TYPE_DATASYNC_CACHE_GET_RESPONSE = "datasync_cache_get_response";
constexpr TStringBuf ITEM_TYPE_DATASYNC_CACHE_SET_REQUEST = "datasync_cache_set_request";
constexpr TStringBuf ITEM_TYPE_DATASYNC_CACHE_DELETE_REQUEST = "datasync_cache_delete_request";

constexpr TStringBuf ITEM_TYPE_FLAGS_JSON_HTTP_REQUEST = "flags_json_request";
constexpr TStringBuf ITEM_TYPE_FLAGS_JSON_HTTP_RESPONSE = "flags_json_response";

constexpr TStringBuf ITEM_TYPE_LAAS_HTTP_REQUEST = "laas_http_request";
constexpr TStringBuf ITEM_TYPE_LAAS_HTTP_RESPONSE = "laas_http_response";
constexpr TStringBuf ITEM_TYPE_LAAS_REQUEST_OPTIONS = "laas_request_options";

constexpr TStringBuf ITEM_TYPE_TVMTOOL_HTTP_REQUEST = "tvmtool_http_request";
constexpr TStringBuf ITEM_TYPE_TVMTOOL_HTTP_RESPONSE = "tvmtool_http_response";

constexpr TStringBuf ITEM_TYPE_FANOUTAUTH_HTTP_REQUEST = "fanoutauth_http_request";
constexpr TStringBuf ITEM_TYPE_FANOUTAUTH_HTTP_RESPONSE = "fanoutauth_http_response";

constexpr TStringBuf ITEM_TYPE_YAMBAUTH_HTTP_REQUEST = "yambauth_http_request";
constexpr TStringBuf ITEM_TYPE_YAMBAUTH_HTTP_RESPONSE = "yambauth_http_response";

constexpr TStringBuf ITEM_TYPE_HTTP_REQUEST_DRAFT = "http_request_draft";
constexpr TStringBuf ITEM_TYPE_DEVICE_ENVIRONMENT_UPDATE_REQUEST = "device_environment_update_request";
constexpr TStringBuf ITEM_TYPE_DEVICE_ENVIRONMENT_UPDATE_RESPONSE = "device_environment_update_response";

// for context_load graph
constexpr TStringBuf ITEM_TYPE_SMARTHOME_UID = "smarthome_uid";

constexpr TStringBuf ITEM_TYPE_ANTIROBOT_INPUT_DATA = "antirobot_data";
constexpr TStringBuf ITEM_TYPE_ANTIROBOT_INPUT_SETTINGS = "antirobot_settings";
constexpr TStringBuf ITEM_TYPE_ANTIROBOT_HTTP_REQUEST = "antirobot_http_request";
constexpr TStringBuf ITEM_TYPE_ANTIROBOT_HTTP_RESPONSE = "antirobot_http_response";

constexpr TStringBuf ITEM_TYPE_CONTEXT_LOAD_REQUEST = "context_load_request";
constexpr TStringBuf ITEM_TYPE_CONTEXT_LOAD_RESPONSE = "context_load_response";

constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_HTTP_REQUEST = "notificator_http_request";
constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_HTTP_RESPONSE = "notificator_http_response";

constexpr TStringBuf ITEM_TYPE_MEMENTO_GET_ALL_OBJECTS_REQUEST = "memento_get_all_objects_request";
constexpr TStringBuf ITEM_TYPE_MEMENTO_HTTP_REQUEST = "memento_http_request";
constexpr TStringBuf ITEM_TYPE_MEMENTO_HTTP_RESPONSE = "memento_http_response";
constexpr TStringBuf ITEM_TYPE_MEMENTO_USER_OBJECTS = "memento_user_objects";

constexpr TStringBuf ITEM_TYPE_MEMENTO_CACHE_GET_REQUEST = "memento_cache_get_request";
constexpr TStringBuf ITEM_TYPE_MEMENTO_CACHE_GET_RESPONSE = "memento_cache_get_response";
constexpr TStringBuf ITEM_TYPE_MEMENTO_CACHE_SET_REQUEST = "memento_cache_set_request";

constexpr TStringBuf ITEM_TYPE_CONTACTS_HTTP_REQUEST = "contacts_http_request";
constexpr TStringBuf ITEM_TYPE_CONTACTS_HTTP_RESPONSE = "contacts_http_response";
constexpr TStringBuf ITEM_TYPE_CONTACTS_PROTO_HTTP_REQUEST = "contacts_proto_http_request";
constexpr TStringBuf ITEM_TYPE_CONTACTS_PROTO_HTTP_RESPONSE = "contacts_proto_http_response";

constexpr TStringBuf ITEM_TYPE_DATASYNC_DEVICE_ID_HTTP_REQUEST = "datasync_device_id_http_request";
constexpr TStringBuf ITEM_TYPE_DATASYNC_DEVICE_ID_HTTP_RESPONSE = "datasync_device_id_http_response";

constexpr TStringBuf ITEM_TYPE_DATASYNC_UUID_HTTP_REQUEST = "datasync_uuid_http_request";
constexpr TStringBuf ITEM_TYPE_DATASYNC_UUID_HTTP_RESPONSE = "datasync_uuid_http_response";

constexpr TStringBuf ITEM_TYPE_QUASARIOT_REQUEST_ALICE_FOR_BUSINESS = "quasariot_request_alice4business";
constexpr TStringBuf ITEM_TYPE_QUASARIOT_RESPONSE_IOT_USER_INFO = "quasariot_response_iot_user_info";

constexpr TStringBuf ITEM_TYPE_QUASARIOT_CACHE_GET_REQUEST = "quasariot_cache_get_request";
constexpr TStringBuf ITEM_TYPE_QUASARIOT_CACHE_GET_RESPONSE = "quasariot_cache_get_response";
constexpr TStringBuf ITEM_TYPE_QUASARIOT_CACHE_SET_REQUEST = "quasariot_cache_set_request";

constexpr TStringBuf ITEM_TYPE_MEGAMIND_SESSION_REQUEST = "mm_session_request";
constexpr TStringBuf ITEM_TYPE_MEGAMIND_SESSION_RESPONSE = "mm_session_response";

constexpr TStringBuf ITEM_TYPE_PREDEFINED_IOT_CONFIG = "predefined_iot_config";
constexpr TStringBuf ITEM_TYPE_PREDEFINED_CONTACTS = "predefined_contacts";

constexpr TStringBuf ITEM_TYPE_AB_EXPERIMENTS_OPTIONS = "ab_experiments_options";
constexpr TStringBuf ITEM_TYPE_FLAGS_INFO = "flags_info";

constexpr TStringBuf ITEM_TYPE_CACHALOT_LOAD_ASR_OPTIONS_PATCH_REQUEST = "cachalot_load_asr_options_patch_request";
constexpr TStringBuf ITEM_TYPE_CACHALOT_LOAD_ASR_OPTIONS_PATCH_RESPONSE = "cachalot_load_asr_options_patch_response";

constexpr TStringBuf ITEM_TYPE_TVM_USER_TICKET = "tvm_user_ticket";
constexpr TStringBuf ITEM_TYPE_BLACKBOX_UID = "blackbox_uid";

// for context_save graph
constexpr TStringBuf ITEM_TYPE_CONTEXT_SAVE_REQUEST = "context_save_request";
// special item for voice_input graph, unlike a normal request, it causes event_exception on fail
constexpr TStringBuf ITEM_TYPE_CONTEXT_SAVE_IMPORTANT_REQUEST = "context_save_important_request";
constexpr TStringBuf ITEM_TYPE_CONTEXT_SAVE_PRE_REQUESTS_INFO = "context_save_pre_requests_info";
constexpr TStringBuf ITEM_TYPE_CONTEXT_SAVE_RESPONSE = "context_save_response";
// special item for voice_input graph, context_save_response from CONTEXT_SAVE_IMPORTANT subgraph renamed to this type
constexpr TStringBuf ITEM_TYPE_CONTEXT_SAVE_IMPORTANT_RESPONSE = "context_save_important_response";

constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_SUBSCRIPTION_HTTP_REQUEST = "notificator_subscription_http_request";
constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_SUBSCRIPTION_HTTP_RESPONSE = "notificator_subscription_http_response";

constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_MARK_AS_READ_HTTP_REQUEST = "notificator_mark_as_read_http_request";
constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_MARK_AS_READ_HTTP_RESPONSE = "notificator_mark_as_read_http_response";

constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_SEND_SUP_PUSH_HTTP_REQUEST = "notificator_send_sup_push_http_request";
constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_SEND_SUP_PUSH_HTTP_RESPONSE = "notificator_send_sup_push_http_response";

constexpr TStringBuf ITEM_TYPE_PERSONAL_CARDS_DISMISS_HTTP_REQUEST = "personal_cards_dismiss_http_request";
constexpr TStringBuf ITEM_TYPE_PERSONAL_CARDS_DISMISS_HTTP_RESPONSE = "personal_cards_dismiss_http_response";

constexpr TStringBuf ITEM_TYPE_PERSONAL_CARDS_ADD_HTTP_REQUEST = "personal_cards_add_http_request";
constexpr TStringBuf ITEM_TYPE_PERSONAL_CARDS_ADD_HTTP_RESPONSE = "personal_cards_add_http_response";

constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_SUP_CARD_HTTP_REQUEST = "notificator_sup_card_http_request";
constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_SUP_CARD_HTTP_RESPONSE = "notificator_sup_card_http_response";

constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_DELETE_PERSONAL_CARDS_HTTP_REQUEST = "notificator_delete_personal_cards_http_request";
constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_DELETE_PERSONAL_CARDS_HTTP_RESPONSE = "notificator_delete_personal_cards_http_response";

constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_PUSH_TYPED_SEMANTIC_FRAME_HTTP_REQUEST = "notificator_push_typed_semantic_frame_http_request";
constexpr TStringBuf ITEM_TYPE_NOTIFICATOR_PUSH_TYPED_SEMANTIC_FRAME_HTTP_RESPONSE = "notificator_push_typed_semantic_frame_http_response";

constexpr TStringBuf ITEM_TYPE_MATRIX_SCHEDULER_ADD_SCHEDULE_ACTION_HTTP_REQUEST = "matrix_scheduler_add_schedule_action_http_request";
constexpr TStringBuf ITEM_TYPE_MATRIX_SCHEDULER_ADD_SCHEDULE_ACTION_HTTP_RESPONSE = "matrix_scheduler_add_schedule_action_http_response";

constexpr TStringBuf ITEM_TYPE_S3_SAVE_USER_AUDIO_HTTP_REQUEST = "s3_save_user_audio_http_request";
constexpr TStringBuf ITEM_TYPE_S3_SAVE_USER_AUDIO_HTTP_RESPONSE = "s3_save_user_audio_http_response";

constexpr TStringBuf ITEM_TYPE_CACHALOT_SAVE_ASR_OPTIONS_PATCH_REQUEST = "cachalot_save_asr_options_patch_request";
constexpr TStringBuf ITEM_TYPE_CACHALOT_SAVE_ASR_OPTIONS_PATCH_RESPONSE = "cachalot_save_asr_options_patch_response";

// for bio_context_save graph
constexpr TStringBuf ITEM_TYPE_YABIO_CONTEXT = "yabio_context";
constexpr TStringBuf ITEM_TYPE_YABIO_CONTEXT_REQUEST = "yabio_context_request";
constexpr TStringBuf ITEM_TYPE_YABIO_CONTEXT_RESPONSE = "yabio_context_response";
constexpr TStringBuf ITEM_TYPE_YABIO_CONTEXT_SAVED = "yabio_context_saved";

constexpr TStringBuf ITEM_TYPE_YABIO_NEW_ENROLLING = "yabio_new_enrolling";
constexpr TStringBuf ITEM_TYPE_YABIO_TEXT = "yabio_text";

// for biometry_update graph
constexpr TStringBuf ITEM_TYPE_REQUEST_BIO_CONTEXT_UPDATE = "request_bio_context_update";

// input for asr,mds nodes and output from tts
constexpr TStringBuf ITEM_TYPE_AUDIO = "audio";
// input for yabio (internal type same as audio)
constexpr TStringBuf ITEM_TYPE_BIO_CLASSIFY_AUDIO = "bio_classify_audio";
// input for yabio (internal type same as audio)
constexpr TStringBuf ITEM_TYPE_BIO_SCORE_AUDIO = "bio_score_audio";

constexpr TStringBuf ITEM_TYPE_ASR_PROTO_RESPONSE = "asr_proto_response";
constexpr TStringBuf ITEM_TYPE_ASR_SPOTTER_VALIDATION = "asr_spotter_validation";
constexpr TStringBuf ITEM_TYPE_ASR_FINISHED = "asr_finished";
constexpr TStringBuf ITEM_TYPE_YABIO_PROTO_RESPONSE = "yabio_proto_response";
constexpr TStringBuf ITEM_TYPE_MUSIC_MATCH_INIT_RESPONSE = "music_match_init_response";
constexpr TStringBuf ITEM_TYPE_MUSIC_MATCH_STREAM_RESPONSE = "music_match_stream_response";

// for tts graph

// protobuf NTts::TBackendRequest (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_BACKEND_REQUEST = "tts_backend_request";
// For items like tts_backend_request_ru_gpu_shitova
// We need this for for correct request routing in tts_backend graph
constexpr TStringBuf ITEM_TYPE_TTS_BACKEND_REQUEST_PREFIX = "tts_backend_request_";

// protobuf NTts::TSplitterRequest (alice/cuttlefish/library/protos/tts.proto)
// Same protobuf but different item types to make apphost logs more readable
constexpr TStringBuf ITEM_TYPE_TTS_REQUEST = "tts_request";
constexpr TStringBuf ITEM_TYPE_TTS_PARTIAL_REQUEST = "tts_partial_request";

// Prefix for s3 audio requests/responses
constexpr TStringBuf ITEM_TYPE_PREFIX_S3_AUDIO_HTTP_REQUEST = "s3_audio_http_request_";
constexpr TStringBuf ITEM_TYPE_PREFIX_S3_AUDIO_HTTP_RESPONSE = "s3_audio_http_response_";

// protobuf NTts::TRequestSenderRequest (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_REQUEST_SENDER_REQUEST = "tts_request_sender_request";

// protobuf NTts::TAggregatorRequest (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_AGGREGATOR_REQUEST = "tts_aggregator_request";

// protobuf NTts::TTimings (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_TIMINGS = "tts_timings";

// tts cache

// protobuf NTts::TCacheSetRequest (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_CACHE_SET_REQUEST = "tts_cache_set_request";
// protobuf NTts::TCacheWarmUpRequest (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_CACHE_WARM_UP_REQUEST = "tts_cache_warm_up_request";

// protobuf NTts::TCacheGetRequest (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_CACHE_GET_REQUEST = "tts_cache_get_request";
// protobuf NTts::TCacheGetResponse (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_CACHE_GET_RESPONSE = "tts_cache_get_response";
// protobuf NTts::TCacheGetResponseStatus (alice/cuttlefish/library/protos/tts.proto)
constexpr TStringBuf ITEM_TYPE_TTS_CACHE_GET_RESPONSE_STATUS = "tts_cache_get_response_status";

// for store_audio graph
constexpr TStringBuf ITEM_TYPE_AUDIO_INFO = "audio_info";
constexpr TStringBuf ITEM_TYPE_AUDIO_CHUNK = "audio_chunk";
constexpr TStringBuf ITEM_TYPE_STORE_AUDIO_DRAFT = "store_audio_draft";
constexpr TStringBuf ITEM_TYPE_STORE_AUDIO_RESPONSE = "store_audio_response";
constexpr TStringBuf ITEM_TYPE_STORE_AUDIO_RESPONSE_SPOTTER = "store_audio_response_spotter";

constexpr TStringBuf ITEM_TYPE_MDS_STORE_SPOTTER_HTTP_REQUEST = "mds_store_spotter_http_request";
constexpr TStringBuf ITEM_TYPE_MDS_STORE_SPOTTER_HTTP_RESPONSE = "mds_store_spotter_http_response";

constexpr TStringBuf ITEM_TYPE_MDS_STORE_STREAM_HTTP_REQUEST = "mds_store_stream_http_request";
constexpr TStringBuf ITEM_TYPE_MDS_STORE_STREAM_HTTP_RESPONSE = "mds_store_stream_http_response";

// for megamind servant
constexpr TStringBuf ITEM_TYPE_MEGAMIND_REQUEST = "mm_request";
constexpr TStringBuf ITEM_TYPE_MEGAMIND_RESPONSE = "mm_response";
constexpr TStringBuf ITEM_TYPE_MEGAMIND_APPLY_REQUEST = "mm_apply_request";
constexpr TStringBuf ITEM_TYPE_MEGAMIND_APPLY_REQUEST_TAG = "mm_apply_request_tag";
constexpr TStringBuf ITEM_TYPE_MEGAMIND_APPLY_RESPONSE = "mm_apply_response";
constexpr TStringBuf ITEM_TYPE_MEGAMIND_RUN_READY = "mm_run_ready";

// smart_activation
constexpr TStringBuf ITEM_TYPE_ACTIVATION_ANNOUNCEMENT_REQUEST = "activation_announcement_request";
constexpr TStringBuf ITEM_TYPE_ACTIVATION_ANNOUNCEMENT_RESPONSE = "activation_announcement_response";
constexpr TStringBuf ITEM_TYPE_ACTIVATION_FINAL_REQUEST = "activation_final_request";
constexpr TStringBuf ITEM_TYPE_ACTIVATION_FINAL_RESPONSE = "activation_final_response";
constexpr TStringBuf ITEM_TYPE_ACTIVATION_SUCCESSFUL = "activation_successful";
constexpr TStringBuf ITEM_TYPE_ACTIVATION_LOG = "activation_log";
constexpr TStringBuf ITEM_TYPE_ACTIVATION_LOG_FINAL = "activation_log_final";

// for uniproxy2 directive
constexpr TStringBuf ITEM_TYPE_UNIPROXY2_DIRECTIVE = "uniproxy2_directive";
constexpr TStringBuf ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG = "uniproxy2_directive_session_log";
constexpr TStringBuf ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS = "uniproxy2_directives_session_logs";
constexpr TStringBuf ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG_FROM_MM_RUN = "uniproxy2_directive_session_log_from_mm_run";
constexpr TStringBuf ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG_FROM_MM_APPLY = "uniproxy2_directive_session_log_from_mm_apply";

// for tesing/debug hacks
constexpr TStringBuf ITEM_TYPE_PREDEFINED_ASR_RESULT = "predefined_asr_result";

// protobuf NAliceProtocol::TEnrollmentHeader (alice/cuttlefish/library/protos/personalization.proto)
constexpr TStringBuf ITEM_TYPE_ENROLLMENT_HEADERS= "enrollment_headers";
constexpr TStringBuf ITEM_TYPE_UPDATE_CLIENT_ENROLLMENT_DIRECTIVE = "update_client_enrollment_directive";

constexpr TStringBuf ITEM_TYPE_VOICEPRINT_MATCH_RESULT = "voiceprint_match_result";
constexpr TStringBuf ITEM_TYPE_VOICEPRINT_NO_MATCH_RESULT = "voiceprint_no_match_result";

// protobuf NAliceProtocol::TFullIncomingAudio (alice/cuttlefish/library/protos/audio_separator.proto)
constexpr TStringBuf ITEM_TYPE_FULL_INCOMING_AUDIO = "full_incoming_audio";

} // namespace NAlice::NCuttlefish
