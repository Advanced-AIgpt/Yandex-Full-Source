LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/analytics/interfaces
    alice/library/json
    contrib/libs/protobuf
)

SRCS(
    builder.cpp
)

END()
