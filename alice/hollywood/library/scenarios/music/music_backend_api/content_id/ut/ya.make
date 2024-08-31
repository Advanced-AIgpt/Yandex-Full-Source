UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_backend_api/content_id)

OWNER(
    stupnik
    g:hollywood
    g:alice
)

SRCS(
    content_id_ut.cpp
    playlist_id_ut.cpp
    track_album_id_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/util
    library/cpp/testing/unittest
)

END()
