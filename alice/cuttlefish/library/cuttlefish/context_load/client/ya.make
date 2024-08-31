LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    antirobot.cpp
    blackbox.cpp
    cachalot_mm_session.cpp
    contacts.cpp
    datasync.cpp
    flags_json.cpp
    iot.cpp
    laas.cpp
    memento.cpp
    selector.cpp
    starter.cpp
)

PEERDIR(
    alice/cachalot/api/protos
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/protos
    apphost/api/service/cpp
    voicetech/library/itags
    voicetech/library/messages
)

END()

RECURSE_FOR_TESTS(ut)
