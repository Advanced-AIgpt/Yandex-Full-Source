LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    enums.cpp
    mapper.cpp
)

PEERDIR(
    contrib/libs/yaml-cpp
    library/cpp/resource
    alice/cuttlefish/library/utils/yaml
)

RESOURCE(
    app_info.yaml /app_info.yaml
    uaas_info.yaml /uaas_info.yaml
)

GENERATE_ENUM_SERIALIZATION(enums.h)
GENERATE_ENUM_SERIALIZATION(surfaces.h)

END()

RECURSE_FOR_TESTS(ut)
