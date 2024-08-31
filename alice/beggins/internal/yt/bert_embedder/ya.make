PROGRAM(embedder)

OWNER(
    alkapov
)

SRCS(
    config.proto
    main.cpp
)

PEERDIR(
    alice/beggins/internal/bert

    library/cpp/getoptpb
    library/cpp/type_info
    library/cpp/yson/node

    mapreduce/yt/client
)

END()
