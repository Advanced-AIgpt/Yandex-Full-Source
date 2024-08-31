PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    contrib/libs/intel/mkl
    alice/boltalka/libs/text_utils
    alice/boltalka/libs/dssm_model
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/util
    library/cpp/getopt/small
    library/cpp/dot_product
    util
)

GENERATE_ENUM_SERIALIZATION(index_type.h)

ALLOCATOR(LF)

END()
