OWNER(g:voicetech-infra)

PROGRAM(cachalot-cli)

SRCS(
    main.cpp
    args.proto
)

PEERDIR(
    library/cpp/getopt
    library/cpp/getoptpb
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

END()
