Y_BENCHMARK()

OWNER(
    g:voicetech-infra
)

SRCS(
    main.cpp
)

RESOURCE(
    input.1.txt /input.1.txt
    input.2.txt /input.2.txt
)

PEERDIR(
    alice/cuttlefish/library/evparse
)

END()
