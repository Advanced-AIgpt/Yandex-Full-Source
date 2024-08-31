LIBRARY()

OWNER(
    klim-roma
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/scenarios/voiceprint/proto
    alice/hollywood/library/scenarios/voiceprint/util

    alice/megamind/protos/blackbox
    alice/megamind/protos/common
    alice/protos/data/scenario/voiceprint
)

JOIN_SRCS_GLOBAL(
    all.cpp
    enroll.cpp
    impl.cpp
    main.cpp
    remove.cpp
    set_my_name.cpp
)

GENERATE_ENUM_SERIALIZATION(main.h)

END()
