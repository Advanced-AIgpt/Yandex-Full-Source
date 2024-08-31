LIBRARY()

OWNER(gusev-p)

SRCS(
    formatting.cpp
    helpers.cpp
)

PEERDIR(
    library/cpp/json
    library/cpp/json/writer
    library/cpp/protobuf/json
)

END()
