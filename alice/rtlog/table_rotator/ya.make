PROGRAM()

OWNER(akhruslan)

SRCS(
    main.cpp
)

PEERDIR(
    alice/rtlog/rthub/protos
    library/cpp/getopt/small
    ydb/public/sdk/cpp/client/ydb_table
    ydb/public/sdk/cpp/client/ydb_value
)

END()
