PROGRAM()

OWNER(g:alice_boltalka)

PEERDIR(
    alice/boltalka/generative/service/proto
    alice/boltalka/generative/inference/core
    alice/boltalka/libs/text_utils
    alice/library/proto
    library/cpp/string_utils/url
)

SRCS(
    main.cpp
)

END()
