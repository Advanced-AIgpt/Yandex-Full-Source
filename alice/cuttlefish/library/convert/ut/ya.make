UNITTEST()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/convert
    alice/cuttlefish/library/convert/ut/proto
    contrib/libs/protobuf
)

SRCS(
    test_json_converts.cpp
    test_json2proto.cpp
    test_rapid_node.cpp
    test_templates.cpp
)

END()
