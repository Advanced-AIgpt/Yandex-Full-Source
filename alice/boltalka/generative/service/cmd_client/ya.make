PROGRAM()

OWNER(g:alice_boltalka)

PEERDIR(
    alice/boltalka/generative/service/proto
    alice/boltalka/generative/inference/core
    alice/boltalka/libs/text_utils
    alice/library/json

    library/cpp/getopt
    library/cpp/http/simple
    library/cpp/langs
)

SRCS(
    main.cpp
)

END()
