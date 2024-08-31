LIBRARY()

OWNER(
    a-square
    akhruslan
    g:hollywood
)

SRCS(
    callback.cpp
    frame.cpp
    slot.cpp
)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

END()

RECURSE_FOR_TESTS(ut)
