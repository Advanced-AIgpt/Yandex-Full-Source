LIBRARY()

OWNER(
    karina-usm
    vitamin-ca
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/http_proxy
    alice/hollywood/library/request
    alice/hollywood/library/scenarios/onboarding/nlg
    alice/hollywood/library/scenarios/onboarding/proto
    alice/library/onboarding
    alice/library/device_state
    alice/library/experiments
    alice/library/video_common
    alice/protos/data/video
    dj/services/alisa_skills/server/proto/client
)

SRCS(
    GLOBAL onboarding.cpp
    greetings_consts.cpp
    greetings_scene.cpp
    memento.cpp
    skillrec_request_helper.cpp
    what_can_you_do_scene.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
