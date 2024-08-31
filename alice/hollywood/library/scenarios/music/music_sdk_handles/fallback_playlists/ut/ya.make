UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_sdk_handles/fallback_playlists)

OWNER(
    sparkle
    g:hollywood
    g:alice
)

SRCS(
    alice/hollywood/library/scenarios/music/music_sdk_handles/fallback_playlists/fallback_playlists_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/music_sdk_handles/fallback_playlists
    library/cpp/testing/unittest
)

END()
