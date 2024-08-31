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
    alice/boltalka/extsearch/base/calc_factors
    mapreduce/yt/client
    mapreduce/yt/interface
    library/cpp/getopt/small
    util
)

ALLOCATOR(LF)

END()
