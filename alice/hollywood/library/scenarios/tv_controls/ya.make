LIBRARY()

OWNER(
    dandex
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/environment_state
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/tv_controls/nlg
    alice/hollywood/library/scenarios/tv_controls/proto
    alice/protos/endpoint/capabilities/screensaver
)

SRCS(
    GLOBAL tv_controls.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)