LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    cloud_speaker.cpp
    speaker.cpp
    voicetech_speaker.cpp
    utils.cpp
)

PEERDIR(
    alice/cuttlefish/library/protos
)

END()

RECURSE_FOR_TESTS(
    tests_canonize
    ut
)
