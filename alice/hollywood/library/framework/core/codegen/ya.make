LIBRARY()

PEERDIR(
    alice/megamind/protos/scenarios
    alice/protos/endpoint
    alice/protos/endpoint/capabilities/audio_file_player
    alice/protos/endpoint/capabilities/battery
    alice/protos/endpoint/capabilities/div_view
    alice/protos/endpoint/capabilities/iot_scenarios
    alice/protos/endpoint/capabilities/layered_div_ui
    alice/protos/endpoint/capabilities/opening_sensor
    alice/protos/endpoint/capabilities/route_manager
    alice/protos/endpoint/capabilities/vibration_sensor
    alice/protos/endpoint/capabilities/water_leak_sensor
)

RUN_PROGRAM(
    alice/tools/jinja2_compiler --proto=${ARCADIA_ROOT}/alice/megamind/protos/scenarios/directives.proto
            --input=directives.cpp.jinja2
            --input=directives.h.jinja2
            --input=server_directives.cpp.jinja2
            --input=server_directives.h.jinja2
            --output=gen_directives.pb.cpp
            --output=gen_directives.pb.h
            --output=gen_server_directives.pb.cpp
            --output=gen_server_directives.pb.h
            --message=TDirective
            --message=TServerDirective
            --include=${ARCADIA_ROOT}/
            --include=${ARCADIA_ROOT}/contrib/libs/protobuf/src/
        IN directives.cpp.jinja2 directives.h.jinja2 server_directives.cpp.jinja2 server_directives.h.jinja2
        OUT_NOAUTO gen_directives.pb.cpp gen_directives.pb.h gen_server_directives.pb.cpp gen_server_directives.pb.h
        ${suf=.h:directives} ${suf=.cpp:directives}
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/megamind/protos/scenarios/directives.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/megamind/protos/scenarios/response.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/capability.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/common.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/endpoint.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/capabilities/battery/capability.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/capabilities/div_view/capability.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/capabilities/iot_scenarios/capability.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/capabilities/opening_sensor/capability.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/capabilities/route_manager/route_manager.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/capabilities/vibration_sensor/capability.pb.h
        OUTPUT_INCLUDES ${ARCADIA_BUILD_ROOT}/alice/protos/endpoint/capabilities/water_leak_sensor/capability.pb.h
)

SRCS(
    gen_directives.pb.cpp
    gen_server_directives.pb.cpp
)

END()
