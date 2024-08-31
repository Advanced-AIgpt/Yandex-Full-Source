PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice_iot
    g:megamind
    g:yandex_io
)

PEERDIR(
    mapreduce/yt/interface/protos
    alice/protos/endpoint
    alice/protos/endpoint/capabilities/alarm
    alice/protos/endpoint/capabilities/audio_file_player
    alice/protos/endpoint/capabilities/battery
    alice/protos/endpoint/capabilities/bio
    alice/protos/endpoint/capabilities/div_view
    alice/protos/endpoint/capabilities/iot_scenarios
    alice/protos/endpoint/capabilities/opening_sensor
    alice/protos/endpoint/capabilities/range_check
    alice/protos/endpoint/capabilities/route_manager
    alice/protos/endpoint/capabilities/vibration_sensor
    alice/protos/endpoint/capabilities/volume
    alice/protos/endpoint/capabilities/water_leak_sensor
    alice/protos/endpoint/capabilities/screensaver
    yandex_io/protos/capabilities
)

SRCS(
   all.proto
)

END()
