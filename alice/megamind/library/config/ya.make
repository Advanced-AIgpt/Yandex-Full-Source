LIBRARY()

OWNER(g:megamind)

PEERDIR(
    contrib/libs/protobuf
    library/cpp/getoptpb
    alice/megamind/library/config/protos
    alice/megamind/library/config/scenario_protos
)

SRCS(
    config.cpp
    proto_fields_visitor.cpp
)

END()

RECURSE_FOR_TESTS(ut)
