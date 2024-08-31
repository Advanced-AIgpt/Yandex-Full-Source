UNITTEST()

OWNER(g:wonderlogs)

SRCS(
    speechkit_utils_ut.cpp
)

PEERDIR(
    alice/wonderlogs/library/common
    alice/wonderlogs/protos
    alice/wonderlogs/sdk/utils
    alice/library/unittest
    alice/library/experiments
    alice/megamind/library/response
    alice/megamind/protos/speechkit
    
    contrib/libs/protobuf
)

END()
