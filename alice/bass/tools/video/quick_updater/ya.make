PROGRAM()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/video_tools/quick_updater
    alice/bass/libs/ydb_config
    library/cpp/getopt
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table
)

SRCS(
    main.cpp
)

END()
