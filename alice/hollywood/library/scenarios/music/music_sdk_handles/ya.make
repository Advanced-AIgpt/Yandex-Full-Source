LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/music
    alice/hollywood/library/scenarios/music/analytics_info
    alice/hollywood/library/scenarios/music/music_backend_api
    alice/hollywood/library/scenarios/music/music_request_builder
    alice/hollywood/library/scenarios/music/music_sdk_handles/fallback_playlists
    alice/hollywood/library/scenarios/music/music_sdk_handles/music_sdk_uri_builder
    alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/music/requests_helper
    alice/hollywood/library/scenarios/music/util
)

JOIN_SRCS(
    all.cpp
    common.cpp
    continue_playlist_setdown_handle.cpp
    continue_prepare_handle.cpp
    continue_render_handle.cpp
    requests_helper.cpp
    run_prepare_handle.cpp
    run_search_content_post_handle.cpp
    web_os_helper.cpp
)

END()

RECURSE_FOR_TESTS(
    fallback_playlists
    ut
)
