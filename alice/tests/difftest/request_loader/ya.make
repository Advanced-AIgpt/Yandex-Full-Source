PROGRAM(loader)

OWNER(sparkle)

PEERDIR(
    alice/megamind/protos/speechkit
    alice/rtlog/rthub/protos
    library/cpp/getopt
    library/cpp/json
    library/cpp/logger/global
    library/cpp/protobuf/json
    mapreduce/yt/client
)

SRCS(
    main.cpp
    request_filters.cpp
)

END()
