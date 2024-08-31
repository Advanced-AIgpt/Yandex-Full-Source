LIBRARY()

OWNER(
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/registry
    alice/library/analytics/common
    alice/library/video_common/hollywood_helpers
    alice/library/proto
    alice/protos/data/tv/watch_list
    library/cpp/uri
    alice/hollywood/library/environment_state
    contrib/libs/googleapis-common-protos
)

SRCS(
    GLOBAL register.cpp
    setup.cpp
    setup_add.cpp
    setup_delete.cpp
    process.cpp 
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
