LIBRARY()

OWNER(
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music/music_backend_api
    alice/hollywood/library/scenarios/music/nlg
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/music/scene/common
    alice/hollywood/library/scenarios/music/scene/fm_radio
    alice/hollywood/library/scenarios/music/scene/player_command
)

SRCS(
    GLOBAL music_dispatcher.cpp
    centaur.cpp
    elari_watch.cpp
    equalizer.cpp
    multiroom_redirect.cpp
    play_less.cpp
    start_multiroom.cpp
    tandem_follower.cpp
)

END()
