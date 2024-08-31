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
    alice/protos/endpoint/capabilities/battery
    alice/protos/endpoint/capabilities/iot_scenarios
    alice/protos/endpoint/capabilities/opening_sensor
    alice/protos/endpoint/capabilities/range_check
    alice/protos/endpoint/capabilities/vibration_sensor
    alice/protos/endpoint/capabilities/water_leak_sensor
)

SRCS(
   all.proto
)

END()
