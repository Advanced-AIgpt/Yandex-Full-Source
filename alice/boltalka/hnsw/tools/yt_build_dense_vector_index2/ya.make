PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/boltalka/hnsw/yt_index_builder2
    library/cpp/getopt
    mapreduce/yt/client
)

GENERATE_ENUM_SERIALIZATION(distance.h)
GENERATE_ENUM_SERIALIZATION(vector_component_type.h)

ALLOCATOR(LF)

END()
