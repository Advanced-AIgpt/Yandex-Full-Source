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

    alice/protos/data/scenario/voiceprint
    alice/protos/endpoint/capabilities/bio
)

JOIN_SRCS_GLOBAL(
    all.cpp
    base.cpp
    collect.cpp
    complete.cpp
    context.cpp
    intro.cpp
    not_started.cpp
    wait_ready.cpp
    wait_username.cpp
)

END()
