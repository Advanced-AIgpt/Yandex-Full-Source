LIBRARY()

OWNER(
    g:hollywood
    g:mediaalice
)

PEERDIR(
    alice/hollywood/library/biometry
    alice/hollywood/library/framework
    alice/hollywood/library/music
    alice/hollywood/library/scenarios/music/proto
    alice/library/experiments
    alice/library/logger
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

SRCS(
    generative.cpp
)

END()
