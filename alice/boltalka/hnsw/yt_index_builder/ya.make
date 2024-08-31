LIBRARY()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    dense_vector_index_builder.h
    index_builder.h
    build_routines.cpp
    index_writer.cpp
)

PEERDIR(
    library/cpp/hnsw/index_builder
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/util
    util
)

END()
