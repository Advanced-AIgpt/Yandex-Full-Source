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
    alice/hollywood/library/scenarios/voiceprint/proto
    alice/hollywood/library/scenarios/voiceprint/state_machine/enroll_run
    alice/hollywood/library/scenarios/voiceprint/state_machine/remove_run
    alice/hollywood/library/scenarios/voiceprint/state_machine/set_my_name
    alice/hollywood/library/scenarios/voiceprint/util

    alice/library/biometry
    alice/library/data_sync
    alice/library/logger

    alice/protos/endpoint/capabilities/bio
)

JOIN_SRCS_GLOBAL(
    all.cpp
    enroll.cpp
    impl.cpp
    main.cpp
    remove.cpp
    set_my_name.cpp
    what_is_my_name.cpp
)

END()
