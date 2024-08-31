LIBRARY()

OWNER(
    g:hollywood
    g:paskills
)

PEERDIR(
    alice/hollywood/library/biometry
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music/proto
    alice/library/experiments
    alice/library/logger
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/memento/proto
)

JOIN_SRCS(
    all.cpp
    bedtime_tales.cpp
    child_age_settings.cpp
    ondemand.cpp
    playlists.cpp
)

END()
