LIBRARY()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    build_routines.cpp
    index_data.cpp
    index_writer.cpp
    chunked_stream.cpp
    internal_build_options.cpp
    types.proto
)

PEERDIR(
    library/cpp/hnsw/index
    library/cpp/hnsw/index_builder
    mapreduce/yt/interface
    mapreduce/yt/interface/protos
    mapreduce/yt/library/table_schema
    mapreduce/yt/util
)

END()
