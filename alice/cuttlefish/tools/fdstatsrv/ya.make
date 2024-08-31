PROGRAM()

OWNER(g:voicetech-infra)

SRCS(
    main.cpp
    metrics.cpp
    updater.cpp
    fs.cpp
    netstat.cpp
    ulimit.cpp
    config.proto
)

PEERDIR(
    library/cpp/neh
    library/cpp/unistat
    library/cpp/getoptpb
)

END()
