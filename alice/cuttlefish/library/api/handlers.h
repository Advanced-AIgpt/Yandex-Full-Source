#pragma once
#include <util/generic/string.h>

namespace NAlice::NCuttlefish {

    // cuttlefish
    extern const TString SERVICE_HANDLE_SYNCHRONIZE_STATE_PRE;
    extern const TString SERVICE_HANDLE_SYNCHRONIZE_STATE_POST;
    extern const TString SERVICE_HANDLE_SYNCHRONIZE_STATE_BLACKBOX_SETDOWN;
    extern const TString SERVICE_HANDLE_FAKE_SYNCHRONIZE_STATE;

    extern const TString SERVICE_HANDLE_RAW_TO_PROTOBUF;
    extern const TString SERVICE_HANDLE_PROTOBUF_TO_RAW;

    extern const TString SERVICE_HANDLE_STREAM_RAW_TO_PROTOBUF;
    extern const TString SERVICE_HANDLE_STREAM_PROTOBUF_TO_RAW;

    extern const TString SERVICE_HANDLE_CONTEXT_LOAD_PRE;
    extern const TString SERVICE_HANDLE_CONTEXT_LOAD_POST;
    extern const TString SERVICE_HANDLE_CONTEXT_LOAD_BLACKBOX_SETDOWN;
    extern const TString SERVICE_HANDLE_CONTEXT_LOAD_PREPARE_LAAS;
    extern const TString SERVICE_HANDLE_CONTEXT_LOAD_PREPARE_FLAGS_JSON;
    extern const TString SERVICE_HANDLE_CONTEXT_LOAD_MAKE_CONTACTS_REQUEST;
    extern const TString SERVICE_HANDLE_FAKE_CONTEXT_LOAD;

    extern const TString SERVICE_HANDLE_CONTEXT_SAVE_PRE;
    extern const TString SERVICE_HANDLE_CONTEXT_SAVE_POST;
    extern const TString SERVICE_HANDLE_FAKE_CONTEXT_SAVE;

    extern const TString SERVICE_HANDLE_GUEST_CONTEXT_LOAD_BLACKBOX_SETDOWN;
    extern const TString SERVICE_HANDLE_STREAM_GUEST_CONTEXT;

    extern const TString SERVICE_HANDLE_STORE_AUDIO_PRE;
    extern const TString SERVICE_HANDLE_STORE_AUDIO_POST;

    extern const TString SERVICE_HANDLE_BIO_CONTEXT_LOAD_POST;
    extern const TString SERVICE_HANDLE_BIO_CONTEXT_SAVE_PRE;
    extern const TString SERVICE_HANDLE_BIO_CONTEXT_SAVE_POST;
    extern const TString SERVICE_HANDLE_BIO_CONTEXT_SYNC;

    extern const TString SERVICE_HANDLE_LOG_SPOTTER;

    extern const TString SERVICE_HANDLE_ANY_INPUT_PRE;

    extern const TString SERVICE_HANDLE_MEGAMIND_RUN;
    extern const TString SERVICE_HANDLE_MEGAMIND_APPLY;

    extern const TString SERVICE_HANDLE_TTS_AGGREGATOR;
    extern const TString SERVICE_HANDLE_TTS_SPLITTER;
    extern const TString SERVICE_HANDLE_TTS_REQUEST_SENDER;

    extern const TString SERVICE_HANDLE_AUDIO_SEPARATOR;

    // asr
    extern const TString SERVICE_HANDLE_ASR;

    // tts
    extern const TString SERVICE_HANDLE_TTS;
    extern const TString SERVICE_HANDLE_TTS_CACHE;

    // music_match
    extern const TString SERVICE_HANDLE_MUSIC_MATCH;

    // yabio
    extern const TString SERVICE_HANDLE_BIO;

    // cachalot
    extern const TString SERVICE_HANDLE_ACTIVATION;
    extern const TString SERVICE_HANDLE_CACHE;
    extern const TString SERVICE_HANDLE_LOCATION;
    extern const TString SERVICE_HANDLE_MM_SESSION;
    extern const TString SERVICE_HANDLE_YABIO_CONTEXT;

    extern const TString SERVICE_HANDLE_SESSION_LOGS_COLLECTOR;
}
