LIBRARY()

OWNER(
    akastornov
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/environment_state
    alice/hollywood/library/framework
    alice/hollywood/library/resources
    alice/hollywood/library/s3_animations
    alice/hollywood/library/scenarios/messenger_call/nlg
    alice/hollywood/library/scenarios/messenger_call/proto
    alice/library/contacts
    alice/library/analytics/common
    alice/library/proto
    alice/library/url_builder
    alice/megamind/protos/scenarios
    alice/nlu/libs/frame
    alice/nlu/libs/normalization
    alice/nlu/libs/tokenization
    alice/protos/data
    alice/protos/data/channel
    alice/protos/endpoint
    library/cpp/langs
    library/cpp/string_utils/quote
    library/cpp/langs
    alice/protos/div
)

RESOURCE(alice/hollywood/library/scenarios/messenger_call/resources/emergency_phones.json emergency_phones.json)

SRCS(
    GLOBAL messenger_call.cpp

    nlu_hint.cpp
    phone_call.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
)
