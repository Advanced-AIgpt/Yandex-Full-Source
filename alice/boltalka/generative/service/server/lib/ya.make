LIBRARY()

OWNER(g:alice_boltalka g:zeliboba)

PEERDIR(
    alice/boltalka/generative/service/proto
    alice/boltalka/generative/service/server/config
    alice/scenarios/lib
    
    library/cpp/getoptpb
)

SRCS(
    utils.cpp
)

END()
