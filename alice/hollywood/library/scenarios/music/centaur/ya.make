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
)

SRCS(
    GLOBAL register.cpp
    collect_main_screen_prepare.cpp
    collect_main_screen_process.cpp
)

END()
