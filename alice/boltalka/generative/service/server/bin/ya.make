PROGRAM(generative_boltalka)

OWNER(g:alice_boltalka g:zeliboba)

PEERDIR(
    alice/boltalka/generative/service/proto
    alice/boltalka/generative/service/server/config
    alice/boltalka/generative/service/server/handlers
    alice/boltalka/generative/service/server/lib

    alice/scenarios/lib

    alice/library/logger

    dict/mt/libs/nn/ynmt/config_helper
    dict/mt/libs/nn/ynmt/extra

    library/cpp/getoptpb
)

SRCS(
    main.cpp
)

END()
