LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
    alice/hollywood/library/scenarios/music/proto
    alice/library/json
    alice/library/url_builder
)

SRCS(
    nlg_data_builder.cpp
)

END()
