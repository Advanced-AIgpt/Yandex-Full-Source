LIBRARY()

OWNER(
    stupnik
    g:hollywood
    g:alice
)

SRCS(
    play_audio.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/music/time_util
    alice/library/json
)

END()
