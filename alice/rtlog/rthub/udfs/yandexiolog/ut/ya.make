UNITTEST_FOR(alice/rtlog/rthub/udfs/yandexiolog)

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
