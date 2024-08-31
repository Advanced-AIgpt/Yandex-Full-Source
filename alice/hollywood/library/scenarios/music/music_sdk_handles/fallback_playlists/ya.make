LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
)

SRCS(
    fallback_playlists.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
