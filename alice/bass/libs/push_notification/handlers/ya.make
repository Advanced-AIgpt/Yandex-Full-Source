LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/client
    alice/bass/libs/push_notification/scheme
    library/cpp/cgiparam
    library/cpp/scheme
    library/cpp/scheme/util
)

SRCS(
    entitysearch_push.cpp
    handler.cpp
    music_push.cpp
    onboarding_push.cpp
    quasar/mm_semantic_frame_push.cpp
    quasar/quasar_pushes.cpp
    quasar/repeat_phrase_push.cpp
    quasar/text_action_push.cpp
    quasar/video_push.cpp
    simple_push.cpp
    taxi_push.cpp
    web_search_push.cpp
)

END()
