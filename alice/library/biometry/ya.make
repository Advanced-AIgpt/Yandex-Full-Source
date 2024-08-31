LIBRARY()

OWNER(g:alice)

SRCS(
    biometry.cpp
)

GENERATE_ENUM_SERIALIZATION(
    biometry.h
)

PEERDIR(
    alice/bass/libs/request
    alice/library/client
    alice/library/passport_api
    alice/megamind/protos/common
)

END()

RECURSE_FOR_TESTS(ut)
