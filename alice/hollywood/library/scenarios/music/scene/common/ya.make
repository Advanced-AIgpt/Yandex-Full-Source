LIBRARY()

OWNER(
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/biometry
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music/music_backend_api
    alice/hollywood/library/scenarios/music/proto
)

SRCS(
    analytics_info.cpp
    audio_play.cpp
    common_args.cpp
    glagol_metadata.cpp
    like_status.cpp
    music_args.cpp
    render.cpp
    request.cpp
    scenario_meta_processor.cpp
    structs.cpp
)

END()
