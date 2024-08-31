PROGRAM(rtlog-agent)

OWNER(
    gusev-p
    g:kwyt
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/rtlog/agent/impl
    library/cpp/getopt
    robot/rthub/misc
    ydb/library/yql/utils/backtrace
)

YQL_LAST_ABI_VERSION()

END()
