LIBRARY()

OWNER(
    stupnik
    g:hollywood
    g:alice
)

JOIN_SRCS(
    all.cpp
    music_queue.cpp
    what_is_playing_answer.cpp
)

GENERATE_ENUM_SERIALIZATION(music_queue.h)

PEERDIR(
    alice/hollywood/library/scenarios/music/biometry
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
    alice/hollywood/library/scenarios/music/music_backend_api/music_config

    alice/hollywood/library/personal_data
    alice/hollywood/library/request

    alice/library/logger
    alice/megamind/protos/scenarios
)

END()
