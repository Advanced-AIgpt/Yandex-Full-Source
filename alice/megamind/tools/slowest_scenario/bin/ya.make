PROGRAM(slowest_scenario)

NO_CLANG_COVERAGE()

OWNER(g:megamind)

PEERDIR(
    alice/library/yt
    alice/library/yt/protos
    alice/megamind/tools/slowest_scenario/library
    library/cpp/getopt
    library/cpp/json/yson
    mapreduce/yt/client
    mapreduce/yt/util
)

SRCS(
    main.cpp
)

END()
