UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_backend_api/content_parsers)

OWNER(
    stupnik
    g:hollywood
    g:alice
)

DATA(arcadia/alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/ut/data/)

SRCS(
    alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/album_parser_ut.cpp
    alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/artist_parser_ut.cpp
    alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/playlist_parser_ut.cpp
    alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/radio_parser_ut.cpp
    alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/track_parser_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/content_parsers
    alice/library/json
    alice/library/music
    alice/library/unittest
    library/cpp/resource
    library/cpp/testing/unittest
)

END()
