PROGRAM()

OWNER(
    movb
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/getopt
    library/cpp/resource
    library/cpp/timezone_conversion
    mapreduce/yt/client
    quality/trailer/suggest/data_structs
    alice/nlu/query_wizard_features/proto
    yweb/blender/lib/yql
)

RESOURCE(
    merge_surpluses.yql /merge_surpluses.yql
    process_daily_tables.yql /process_daily_tables.yql
    combine_tables.yql /combine_tables.yql
)

SRCS(
    main.cpp
)

END()
