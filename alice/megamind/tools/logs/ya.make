PROGRAM(megamind_logs)

NO_CLANG_COVERAGE()

OWNER(g:megamind)

PEERDIR(
    alice/library/yt
    alice/library/yt/protos
    alice/megamind/tools/logs/protos
    contrib/libs/re2
    library/cpp/compute_graph
    library/cpp/getopt
    mapreduce/yt/client
    mapreduce/yt/util
)

SRCS(
    errors_stats.cpp
    main.cpp
    requests_stats.cpp
    timing_stats.cpp
    util.cpp
)

END()
