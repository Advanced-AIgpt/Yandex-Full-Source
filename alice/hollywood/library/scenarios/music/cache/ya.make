LIBRARY()

OWNER(
    amullanurov
    bvdvlg
    klim-roma
    sparkle
    zhigan
)

PEERDIR(
    alice/hollywood/library/base_hw_service
    alice/hollywood/library/registry
    alice/hollywood/library/scenarios/music/proto
    alice/library/cachalot_cache
)

SRCS(
    GLOBAL register.cpp
    cache_prepare_get.cpp
    cache_prepare_set.cpp
    cache_process_get.cpp
    cache_process_set.cpp
    common.cpp
)

END()
