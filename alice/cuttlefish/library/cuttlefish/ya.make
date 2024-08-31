LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    run.cpp
    services.cpp
)

PEERDIR(
    apphost/api/service/cpp
    library/cpp/terminate_handler

    alice/cuttlefish/library/api
    alice/cuttlefish/library/apphost

    alice/cuttlefish/library/cuttlefish/any_input
    alice/cuttlefish/library/cuttlefish/audio_separator
    alice/cuttlefish/library/cuttlefish/bio_context_load
    alice/cuttlefish/library/cuttlefish/bio_context_save
    alice/cuttlefish/library/cuttlefish/bio_context_sync
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/cuttlefish/config
    alice/cuttlefish/library/cuttlefish/context_load
    alice/cuttlefish/library/cuttlefish/context_save
    alice/cuttlefish/library/cuttlefish/converter
    alice/cuttlefish/library/cuttlefish/fake_synchronize_state
    alice/cuttlefish/library/cuttlefish/guest_context_load
    alice/cuttlefish/library/cuttlefish/log_spotter
    alice/cuttlefish/library/cuttlefish/megamind
    alice/cuttlefish/library/cuttlefish/session_logs_collector
    alice/cuttlefish/library/cuttlefish/store_audio
    alice/cuttlefish/library/cuttlefish/stream_converter
    alice/cuttlefish/library/cuttlefish/synchronize_state
    alice/cuttlefish/library/cuttlefish/tts/aggregator/service
    alice/cuttlefish/library/cuttlefish/tts/request_sender
    alice/cuttlefish/library/cuttlefish/tts/splitter

    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/mlock

    alice/protos/api/meta

    library/cpp/openssl/crypto
)

END()

RECURSE(
    any_input
    bio_context_load
    bio_context_save
    bio_context_sync
    common
    config
    context_load
    context_save
    converter
    guest_context_load
    log_spotter
    megamind
    store_audio
    stream_converter
    stream_servant_base
    synchronize_state
    tts
)
