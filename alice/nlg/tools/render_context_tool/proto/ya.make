LIBRARY()

OWNER(g:alice)

PEERDIR(
    library/cpp/getoptpb/proto

    alice/protos/data/language
)

SRCS(
    config.proto
)

END()
