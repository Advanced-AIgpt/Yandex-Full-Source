LIBRARY(tv_home_scenario)

OWNER(
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/tv_home/nlg
    alice/protos/data/video
    alice/library/proto
)

SRCS(
    tv_home_run.cpp
    tv_home_continue.cpp
    tv_home_render.cpp
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
