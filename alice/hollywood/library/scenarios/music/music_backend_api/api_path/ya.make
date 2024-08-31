LIBRARY()

OWNER(
    stupnik
    g:alice
    g:hollywood
)

SRCS(
    api_path.cpp
)

PEERDIR(
    alice/hollywood/library/request
    alice/hollywood/library/scenarios/music/biometry
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/time_util
    alice/hollywood/library/scenarios/music/util
    alice/library/json
    alice/megamind/protos/scenarios
    library/cpp/cgiparam
    library/cpp/string_utils/quote
)

END()
