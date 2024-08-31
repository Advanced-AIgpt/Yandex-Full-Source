UNITTEST_FOR(alice/rtlog/rthub/udfs/rtlog)

OWNER(g:megamind)

PEERDIR(
    library/cpp/testing/unittest
    ydb/library/yql/public/udf/service/stub
)

SRCS(
    parser_ut.cpp
)

YQL_LAST_ABI_VERSION()

END()
