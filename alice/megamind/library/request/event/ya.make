LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    alice/megamind/library/common
    alice/megamind/library/models/directives
    alice/megamind/library/serializers
    alice/megamind/library/util
)

SRCS(
    common.cpp
    event.cpp
    image_input_event.cpp
    music_input_event.cpp
    server_action_event.cpp
    suggested_input_event.cpp
    text_input_event.cpp
    voice_input_event.cpp
)

END()

RECURSE_FOR_TESTS(ut)
