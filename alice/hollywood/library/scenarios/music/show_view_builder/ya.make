LIBRARY()

OWNER(
    sparkle
    g:alice_scenarios
    g:hollywood
)

SRCS(
    show_view_builder.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/content_parsers
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/music/requests_helper
)

END()
