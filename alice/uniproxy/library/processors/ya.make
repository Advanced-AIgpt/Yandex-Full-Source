PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/cuttlefish/library/protos
    alice/megamind/protos/common
    alice/uniproxy/library/backends_asr
    alice/uniproxy/library/backends_bio
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/backends_ctxs
    alice/uniproxy/library/backends_tts
    alice/uniproxy/library/events
    alice/uniproxy/library/experiments
    alice/uniproxy/library/extlog
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/matrix_api
    alice/uniproxy/library/messenger
    alice/uniproxy/library/musicstream
    alice/uniproxy/library/notificator
    alice/uniproxy/library/perf_tester
    alice/uniproxy/library/personal_cards
    alice/uniproxy/library/personal_data
    alice/uniproxy/library/settings
    alice/uniproxy/library/uaas
    alice/uniproxy/library/utils
    alice/uniproxy/library/vins
    alice/uniproxy/library/vins_context_storage
    yweb/webdaemons/icookiedaemon/icookie_lib/utils_py

    alice/cuttlefish/library/protos
    library/python/cityhash
)

PY_SRCS(
    __init__.py
    asr.py
    base_event_processor.py
    biometry.py
    log.py
    messenger.py
    soundrecorder.py
    system.py
    tts.py
    ttsbackwardapphost.py
    uniproxy2.py
    vins.py
)

END()

RECURSE_FOR_TESTS(ut)
