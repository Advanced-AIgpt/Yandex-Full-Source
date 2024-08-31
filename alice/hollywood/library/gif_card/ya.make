LIBRARY()

OWNER(
    jan-fazli
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/gif_card/proto
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/library/logger
    library/cpp/json
)

SRCS(
    gif_card.cpp
)

COMPILE_NLG(
    gif_card.nlg
)

END()
