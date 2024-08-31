LIBRARY()

OWNER(
    stupnik
    g:hollywood
    g:alice
)

JOIN_SRCS(
    all.cpp
    album_parser.cpp
    artist_parser.cpp
    common.cpp
    content_parser.cpp
    generative_parser.cpp
    playlist_parser.cpp
    radio_parser.cpp
    track_parser.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
    alice/hollywood/library/scenarios/music/proto
    alice/library/logger
)

END()
