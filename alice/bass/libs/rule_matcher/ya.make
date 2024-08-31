LIBRARY()

OWNER(g:alice)

PEERDIR(
    library/cpp/json
    library/cpp/scheme
    kernel/qtree/richrequest
    kernel/remorph/input/richtree
    kernel/remorph/tokenlogic
)

SRCS(
    tokenlogic_matcher.cpp
)

END()
