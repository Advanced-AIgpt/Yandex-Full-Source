PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    context_transform.cpp
    layers.cpp
    main.cpp
    nlg_model.cpp
    rnn_model.cpp
    server.cpp
    thread_pool.cpp
)

PEERDIR(
    library/cpp/cgiparam
    library/cpp/getopt/small
    ml/dssm/lib
    quality/deprecated/Misc
    quality/deprecated/net_http
    util
)

ALLOCATOR(LF)

END()
