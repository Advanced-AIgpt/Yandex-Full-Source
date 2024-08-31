LIBRARY()

OWNER(
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music/music_backend_api
    alice/hollywood/library/scenarios/music/proto
)

SRCS(
    common.cpp
    remove_dislike.cpp
    remove_like.cpp
    repeat.cpp
    rewind.cpp
    send_song_text.cpp
    shuffle.cpp
    songs_by_this_artist.cpp
    what_album_is_this_song_from.cpp
    what_is_playing.cpp
    what_is_this_song_about.cpp
    what_year_is_this_song.cpp
)

END()
