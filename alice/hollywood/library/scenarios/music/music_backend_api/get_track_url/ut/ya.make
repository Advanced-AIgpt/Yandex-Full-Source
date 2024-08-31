UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_backend_api/get_track_url)

OWNER(
    stupnik
    g:hollywood
    g:alice
)

DATA(arcadia/alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/ut/data/)

SRCS(
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/xml_resp_parser_ut.cpp
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/track_quality_selector_ut.cpp
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/download_info_parser_ut.cpp
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/signature_token_ut.cpp
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/track_url_builder_ut.cpp
)

PEERDIR(
    library/cpp/resource
    library/cpp/testing/unittest
)

END()
