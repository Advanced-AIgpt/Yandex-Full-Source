PROGRAM()

OWNER(g:voicetech-infra)

SRCS(
    main.cpp
)

PEERDIR(
    alice/cuttlefish/library/api
    alice/cuttlefish/library/cuttlefish_client
    alice/cuttlefish/library/logging
    voicetech/library/messages
)

END()
