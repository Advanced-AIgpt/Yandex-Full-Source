PROGRAM()

OWNER(
    montaglue
    vl-trifonov
    g:alice_quality
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/protos/api/nlu
    kernel/alice/begemot_nlu_factors_info/fill_factors
    kernel/factor_storage
    library/cpp/getopt
    mapreduce/yt/client
)

END()
