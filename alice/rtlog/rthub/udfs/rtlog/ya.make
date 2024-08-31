YQL_UDF(rtlog_udf)

YQL_ABI_VERSION(
    2
    8
    0
)

OWNER(g:megamind)

SRCS(
    parser.cpp
    rtlog_udf.cpp
)

PEERDIR(
    alice/rtlog/common
    alice/rtlog/rthub/udfs/rtlog/parser
    library/cpp/protobuf/util
    robot/rthub/yql/generic_protos
    ydb/library/yql/minikql
    ydb/library/yql/public/udf
    yql/library/protobuf_udf
)

END()

RECURSE_FOR_TESTS(
    ut
    util
)
