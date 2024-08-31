LIBRARY()

OWNER(
    klim-roma
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/biometry
    alice/hollywood/library/environment_state
    alice/hollywood/library/personal_data
    alice/hollywood/library/request
    alice/hollywood/library/resources
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/voiceprint/proto

    alice/library/geo
    alice/library/proto

    alice/protos/data/scenario/voiceprint
    alice/protos/endpoint/capabilities/bio
)

JOIN_SRCS(
    all.cpp
    util.cpp
)

GENERATE_ENUM_SERIALIZATION(util.h)

END()
