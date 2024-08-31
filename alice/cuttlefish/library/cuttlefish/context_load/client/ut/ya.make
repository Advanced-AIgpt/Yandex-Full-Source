UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/context_load/client)

OWNER(g:voicetech-infra)

SRCS(
    cachalot_mm_session_ut.cpp
    common.cpp
    contacts_ut.cpp
    datasync_ut.cpp
    flags_json_ut.cpp
    iot_ut.cpp
    laas_ut.cpp
    memento_ut.cpp
    selector_ut.cpp
    starter_ut.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    apphost/lib/service_testing
)

END()
