UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_backend_api/api_path)

OWNER(
    stupnik
    g:alice
    g:hollywood
)

SRCS(
    alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path_ut.cpp
)

PEERDIR(
    alice/hollywood/library/request
    alice/hollywood/library/scenarios/music/biometry
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/library/json
    alice/megamind/protos/scenarios
)

END()
