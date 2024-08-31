LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/request/event
    alice/megamind/protos/common
)

SRCS(
    event.cpp
    composite.cpp
    view.cpp
)

END()

RECURSE_FOR_TESTS(ut)
