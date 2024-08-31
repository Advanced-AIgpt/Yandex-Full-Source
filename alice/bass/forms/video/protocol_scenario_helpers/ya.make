LIBRARY()

OWNER(nkodosov)

PEERDIR(
    alice/bass/libs/logging_v2
    alice/library/frame
    alice/library/video_common
    alice/megamind/library/experiments
    alice/megamind/protos/common
    alice/megamind/protos/common/required_messages
    alice/megamind/protos/scenarios
    alice/protos/api/nlu/generated
    contrib/libs/protobuf
)

SRCS(
    intents.cpp
    intent_classifier.cpp
    request_creator.cpp
    utils.cpp
)

END()
