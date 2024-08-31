LIBRARY()

OWNER(g:alice)

PEERDIR(
    contrib/libs/protobuf
    library/cpp/json
)

SRCS(
    json.cpp
)

END()
