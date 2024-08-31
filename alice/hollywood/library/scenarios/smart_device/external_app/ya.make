LIBRARY(smart_device_external_app)

OWNER(g:smarttv)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/environment_state
    alice/hollywood/library/registry
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/smart_device/external_app/nlg
    alice/library/analytics/common
    alice/megamind/protos/common
    alice/protos/data/scenario
    alice/protos/data/scenario/centaur
    alice/protos/endpoint
    tools/enum_parser/enum_serialization_runtime
)

SRCS(
    common_run_render.cpp
    web_os_helper.cpp
    GLOBAL register.cpp
)

GENERATE_ENUM_SERIALIZATION(common_run_render.h)

END()

RECURSE_FOR_TESTS(
    it2
)
