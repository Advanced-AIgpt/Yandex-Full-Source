PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    voicetech/library/evlogdump

    # Events
    alice/cachalot/events
    alice/gproxy/library/events
    alice/rtlog/protos
    voicetech/library/idl/log
)

SRCS(
    main.cpp
)

END()
