Y_BENCHMARK()
OWNER(g:voicetech-infra)

SRCS(
    main.cpp
)

RESOURCE(
    json_src.json /json_src.json
)

PEERDIR(
    library/cpp/protobuf/json
    alice/cuttlefish/library/convert
    alice/cuttlefish/library/convert/bench/proto
)

END()
