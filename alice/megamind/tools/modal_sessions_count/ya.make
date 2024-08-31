PROGRAM(megamind_modal_reqstats)

NO_CLANG_COVERAGE()

OWNER(g:megamind)

PEERDIR(
    alice/library/yt
    alice/library/yt/protos
    alice/megamind/library/classifiers/util
    alice/megamind/library/config
    alice/megamind/library/session
    alice/megamind/tools/modal_sessions_count/protos
    library/cpp/getopt
    mapreduce/yt/client
    mapreduce/yt/util
)

SRCS(
    config_helpers.cpp
    main.cpp
    mapper.cpp
)

END()
