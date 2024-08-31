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
    args_builder.cpp
    content_parser.cpp
    fm_radio.cpp
    request.cpp
)

END()
