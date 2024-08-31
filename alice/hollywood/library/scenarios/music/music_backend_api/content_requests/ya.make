LIBRARY()

OWNER(
    stupnik
    g:hollywood
    g:alice
)

SRCS(
    content_requests.cpp
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music/music_backend_api/api_path
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/music_request_builder
    alice/hollywood/library/scenarios/music/proto
    library/cpp/string_utils/base64
)

END()
