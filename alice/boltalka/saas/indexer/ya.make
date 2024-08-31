PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/logger/global
    mapreduce/lib
    saas/util
    saas/api
    saas/api/indexing_client
    saas/api/mr_client
    saas/util/logging
    library/cpp/getopt
    library/cpp/deprecated/atomic
)

END()
