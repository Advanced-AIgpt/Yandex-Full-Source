PROGRAM(build_index)

SRCS(
    build.cpp
)

PEERDIR(
    library/cpp/getopt/small
    library/cpp/hnsw/index
    library/cpp/hnsw/index_builder
    library/cpp/scheme
    util
)

ALLOCATOR(LF)

END()
