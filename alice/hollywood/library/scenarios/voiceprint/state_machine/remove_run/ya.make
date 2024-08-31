LIBRARY()

OWNER(
    klim-roma
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/biometry
    alice/hollywood/library/environment_state
    alice/hollywood/library/personal_data
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/voiceprint/proto
    alice/hollywood/library/scenarios/voiceprint/util

    alice/library/biometry
    alice/library/data_sync
    alice/library/logger

    alice/protos/endpoint/capabilities/bio
)

JOIN_SRCS_GLOBAL(
    all.cpp
    base.cpp
    client_biometry_init.cpp
    context.cpp
    not_started.cpp
    server_biometry_init.cpp
    wait_confirm.cpp
)

GENERATE_ENUM_SERIALIZATION(remove_run.h)

END()