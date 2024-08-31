#pragma once
#include <util/generic/strbuf.h>

namespace NAlice::NCuttlefish {


// for context_load subgraph
constexpr TStringBuf EDGE_FLAG_SMARTHOME_UID = "has_smarthome_uid";
constexpr TStringBuf EDGE_FLAG_MEGAMIND_SESSION_REQUEST = "has_mm_session_request";
constexpr TStringBuf EDGE_FLAG_PREDEFINED_CONTACTS = "has_predefined_contacts";


// for context_save subgraph in voice_input
constexpr TStringBuf EDGE_FLAG_CONTEXT_SAVE_NEED_FULL_INCOMING_AUDIO = "context_save_need_full_incoming_audio";
constexpr TStringBuf EDGE_FLAG_CONTEXT_SAVE_IMPORTANT_NEED_FULL_INCOMING_AUDIO = "context_save_important_need_full_incoming_audio";


// Edge flags for filtration of sources in context_{load,save} subgraphs of voice_input graph
constexpr TStringBuf EDGE_FLAG_CONTACTS_JSON = "contacts_json";
constexpr TStringBuf EDGE_FLAG_CONTACTS_PROTO = "contacts_proto";
constexpr TStringBuf EDGE_FLAG_CONTACTS_AFTER_FLAGS = "contacts_after_flags";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION = "load_context_source_cachalot_mm_session";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION_CROSS_DC = "load_context_source_cachalot_mm_session_cross_dc";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION_LOCAL_DC = "load_context_source_cachalot_mm_session_local_dc";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON = "load_context_source_contacts_json";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO = "load_context_source_contacts_proto";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_DATASYNC = "load_context_source_datasync";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_FLAGS_JSON = "load_context_source_flags_json";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_IOT_USER_INFO = "load_context_source_iot_user_info";
constexpr TStringBuf EDGE_FLAG_LOAD_CONTEXT_SOURCE_MEMENTO = "load_context_source_memento";
constexpr TStringBuf EDGE_FLAG_LOAD_GUEST_CONTEXT_SOURCE_DATASYNC = "load_guest_context_source_datasync";
constexpr TStringBuf EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION = "save_context_source_cachalot_mm_session";
constexpr TStringBuf EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION_CROSS_DC = "save_context_source_cachalot_mm_session_cross_dc";
constexpr TStringBuf EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION_LOCAL_DC = "save_context_source_cachalot_mm_session_local_dc";
constexpr TStringBuf EDGE_FLAG_SAVE_CONTEXT_SOURCE_DATASYNC = "save_context_source_datasync";
constexpr TStringBuf EDGE_FLAG_SAVE_CONTEXT_SOURCE_MEMENTO = "save_context_source_memento";


// for asr, tts, music_match subgraphs
constexpr TStringBuf EDGE_FLAG_ASR = "has_asr";
constexpr TStringBuf EDGE_FLAG_TTS = "has_tts";
constexpr TStringBuf EDGE_FLAG_MUSIC_MATCH = "has_music_match";
constexpr TStringBuf EDGE_FLAG_SMART_ACTIVATION = "has_smart_activation";
constexpr TStringBuf EDGE_FLAG_BIO_CLASSIFY = "has_bio_classify";
constexpr TStringBuf EDGE_FLAG_BIO_SCORE = "has_bio_score";
constexpr TStringBuf EDGE_FLAG_LOAD_BIO_CONTEXT = "has_load_bio_context";
constexpr TStringBuf EDGE_FLAG_USE_ASR_CONTACTS = "use_asr_contacts";


// following flags should not be used in edge expressions!
constexpr TStringBuf EDGE_FLAG_DO_NOT_WRITE_INFO_TO_RTLOG = "do_not_write_info_to_rtlog";
constexpr TStringBuf EDGE_FLAG_DO_NOT_WRITE_INFO_TO_EVENTLOG = "do_not_write_info_to_eventlog";


}  // namespace NAlice::NCuttlefish
