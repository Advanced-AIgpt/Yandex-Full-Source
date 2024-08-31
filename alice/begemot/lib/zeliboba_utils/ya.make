LIBRARY()

OWNER(
    vklyukin
    g:zeliboba
)

PEERDIR(
    alice/boltalka/generative/service/proto
    alice/boltalka/generative/service/server/config
    alice/library/proto
    library/cpp/json
    search/begemot/core
)

SRCS(
    scorer_config.cpp
)

END()
