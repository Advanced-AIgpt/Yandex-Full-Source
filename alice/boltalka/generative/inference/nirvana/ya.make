PROGRAM(generative_boltalka_for_nirvana)

OWNER(
    artemkorenev
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/boltalka/generative/inference/core
    alice/boltalka/libs/text_utils
    mapreduce/yt/client
    library/cpp/getopt
    library/cpp/threading/future
)

END()
