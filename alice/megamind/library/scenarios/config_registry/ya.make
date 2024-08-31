LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/config
    alice/megamind/library/util
    alice/megamind/protos/common
    library/cpp/resource
    library/cpp/protobuf/util
    contrib/libs/protobuf
)

SRCS(
    config_registry.cpp
    config_validator.cpp
)

END()

RECURSE_FOR_TESTS(ut)
