LIBRARY()

OWNER(
    the0
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/frame_filler/lib
    alice/hollywood/library/http_requester
    alice/hollywood/library/registry
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/goodwin/handlers
    alice/library/scenarios/data_sources
)

SRCS(
    GLOBAL goodwin.cpp
)

END()

RECURSE(
    handlers
)
