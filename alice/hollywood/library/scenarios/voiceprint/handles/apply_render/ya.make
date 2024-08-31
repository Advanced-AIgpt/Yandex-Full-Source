LIBRARY()

OWNER(
    klim-roma
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/scenarios/voiceprint/util
)

JOIN_SRCS_GLOBAL(
    all.cpp
    main.cpp
)

GENERATE_ENUM_SERIALIZATION(main.h)

END()
