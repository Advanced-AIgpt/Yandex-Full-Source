PROGRAM()

OWNER(g:alice_boltalka)

PEERDIR(
    alice/boltalka/generative/service/proto
    alice/boltalka/generative/inference/core
    alice/boltalka/libs/text_utils

    library/cpp/http/simple
)

SRCS(
    main.cpp
)

END()
