PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    context_transform.cpp
    dssm_model.cpp
    main.cpp
    nlg_model.cpp
    rnn_model.cpp
    server.cpp
    thread_pool.cpp
)

PEERDIR(
    cv/imgclassifiers/danet/backend/cpu_mkl
    cv/imgclassifiers/danet/common
    cv/imgclassifiers/danet/common/danet_env
    cv/imgclassifiers/danet/data_provider/ext
    cv/imgclassifiers/danet/nnlib/tester
    dict/word2vec/model
    dict/word2vec/util/analogy/invmi
    library/cpp/cgiparam
    library/cpp/getopt/small
    quality/deprecated/Misc
    quality/deprecated/net_http
    util
)

ALLOCATOR(LF)

END()
