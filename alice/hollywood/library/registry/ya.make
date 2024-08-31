LIBRARY()

OWNER(
    akhruslan
    g:hollywood
)

SRCS(
    hw_service_registry.cpp
    registry.cpp
    secret_registry.cpp
)

PEERDIR(
    alice/hollywood/library/base_hw_service
    alice/hollywood/library/base_scenario
    alice/hollywood/library/config
    alice/hollywood/library/framework
    library/cpp/protobuf/util
    library/cpp/string_utils/secret_string
)

END()
