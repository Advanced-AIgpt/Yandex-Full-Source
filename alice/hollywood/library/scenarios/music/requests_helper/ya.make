LIBRARY()

OWNER(
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/api_path
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url
    alice/hollywood/library/scenarios/music/music_request_builder
    alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/music/util
)

JOIN_SRCS(
    all.cpp
    requests_helper.cpp
    requests_helper_base.cpp
)

END()

RECURSE_FOR_TESTS(ut)
