PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/hnsw/index
    alice/boltalka/hnsw/yt_index_builder
    library/cpp/getopt/small
    util
)

GENERATE_ENUM_SERIALIZATION(distance.h)
GENERATE_ENUM_SERIALIZATION(vector_component_type.h)

ALLOCATOR(LF)

END()
