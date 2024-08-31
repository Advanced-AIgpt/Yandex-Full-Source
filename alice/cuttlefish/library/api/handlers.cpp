#include "handlers.h"

namespace NAlice::NCuttlefish {

    // cuttlefish
    const TString SERVICE_HANDLE_SYNCHRONIZE_STATE_PRE = "/synchronize_state-pre";
    const TString SERVICE_HANDLE_SYNCHRONIZE_STATE_POST = "/synchronize_state-post";
    const TString SERVICE_HANDLE_SYNCHRONIZE_STATE_BLACKBOX_SETDOWN = "/synchronize_state-blackbox_setdown";
    const TString SERVICE_HANDLE_FAKE_SYNCHRONIZE_STATE = "/fake_synchronize_state";

    const TString SERVICE_HANDLE_RAW_TO_PROTOBUF = "/raw_to_protobuf";
    const TString SERVICE_HANDLE_PROTOBUF_TO_RAW = "/protobuf_to_raw";

    const TString SERVICE_HANDLE_STREAM_RAW_TO_PROTOBUF = "/stream_raw_to_protobuf";
    const TString SERVICE_HANDLE_STREAM_PROTOBUF_TO_RAW = "/stream_protobuf_to_raw";

    const TString SERVICE_HANDLE_CONTEXT_LOAD_PRE = "/context_load-pre";
    const TString SERVICE_HANDLE_CONTEXT_LOAD_POST = "/context_load-post";
    const TString SERVICE_HANDLE_CONTEXT_LOAD_BLACKBOX_SETDOWN = "/context_load-blackbox_setdown";
    const TString SERVICE_HANDLE_CONTEXT_LOAD_PREPARE_LAAS = "/context_load-prepare_laas";
    const TString SERVICE_HANDLE_CONTEXT_LOAD_PREPARE_FLAGS_JSON = "/context_load-prepare_flags_json";
    const TString SERVICE_HANDLE_CONTEXT_LOAD_MAKE_CONTACTS_REQUEST = "/context_load-make_contacts_request";

    const TString SERVICE_HANDLE_FAKE_CONTEXT_LOAD = "/fake_context_load";

    const TString SERVICE_HANDLE_GUEST_CONTEXT_LOAD_BLACKBOX_SETDOWN = "/guest_context_load-blackbox_setdown";
    const TString SERVICE_HANDLE_STREAM_GUEST_CONTEXT = "/stream_guest_context";

    const TString SERVICE_HANDLE_CONTEXT_SAVE_PRE = "/context_save-pre";
    const TString SERVICE_HANDLE_CONTEXT_SAVE_POST = "/context_save-post";
    const TString SERVICE_HANDLE_FAKE_CONTEXT_SAVE = "/fake_context_save";

    const TString SERVICE_HANDLE_STORE_AUDIO_PRE = "/store_audio-pre";
    const TString SERVICE_HANDLE_STORE_AUDIO_POST = "/store_audio-post";

    const TString SERVICE_HANDLE_BIO_CONTEXT_LOAD_POST = "/bio_context_load-post";
    const TString SERVICE_HANDLE_BIO_CONTEXT_SAVE_PRE = "/bio_context_save-pre";
    const TString SERVICE_HANDLE_BIO_CONTEXT_SAVE_POST = "/bio_context_save-post";
    const TString SERVICE_HANDLE_BIO_CONTEXT_SYNC = "/bio_context_sync";

    const TString SERVICE_HANDLE_LOG_SPOTTER = "/log_spotter";

    const TString SERVICE_HANDLE_ANY_INPUT_PRE = "/any_input-pre";

    const TString SERVICE_HANDLE_MEGAMIND_RUN = "/megamind_run";
    const TString SERVICE_HANDLE_MEGAMIND_APPLY = "/megamind_apply";

    const TString SERVICE_HANDLE_TTS_AGGREGATOR = "/tts_aggregator";
    const TString SERVICE_HANDLE_TTS_REQUEST_SENDER = "/tts_request_sender";
    const TString SERVICE_HANDLE_TTS_SPLITTER = "/tts_splitter";

    const TString SERVICE_HANDLE_AUDIO_SEPARATOR = "/audio_separator";

    // asr
    const TString SERVICE_HANDLE_ASR = "/asr";

    // tts
    const TString SERVICE_HANDLE_TTS = "/tts";
    const TString SERVICE_HANDLE_TTS_CACHE = "/tts_cache";

    // music_match
    const TString SERVICE_HANDLE_MUSIC_MATCH = "/music_match";

    // yabio
    const TString SERVICE_HANDLE_BIO = "/bio";

    // cachalot
    const TString SERVICE_HANDLE_ACTIVATION = "/activation";
    const TString SERVICE_HANDLE_CACHE = "/cache";
    const TString SERVICE_HANDLE_LOCATION = "/location";
    const TString SERVICE_HANDLE_MM_SESSION = "/mm_session";
    const TString SERVICE_HANDLE_YABIO_CONTEXT = "/yabio_context";

    const TString SERVICE_HANDLE_SESSION_LOGS_COLLECTOR = "/session_logs_collector";
}
