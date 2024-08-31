LIBRARY()

OWNER(
    akhruslan
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/registry
    alice/hollywood/library/resources
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/sssss/proto
    alice/library/logger
)

SRCS(
    GLOBAL sssss.cpp
)

END()
