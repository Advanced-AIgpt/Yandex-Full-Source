PROGRAM()

OWNER(
    dan-anastasev
)

PEERDIR(
    alice/nlu/libs/embedder
    alice/nlu/libs/encoder
    library/cpp/resource
)

SRCS(
    main.cpp
)

END()

RECURSE(
    model
)
