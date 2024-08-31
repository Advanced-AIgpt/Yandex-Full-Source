PROGRAM()

OWNER(g:voicetech-infra)

SRCS(
    main.cpp
)

PEERDIR(
    alice/cuttlefish/library/api
    alice/cuttlefish/library/cachalot_client
    alice/cuttlefish/library/logging
    library/cpp/protobuf/json
    voicetech/library/proto_api
)

END()
