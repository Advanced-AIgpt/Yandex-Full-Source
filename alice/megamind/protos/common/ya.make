PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/censor/protos
    alice/library/client/protos
    alice/library/restriction_level/protos
    alice/protos/data
    alice/protos/data/device
    alice/protos/data/entity_meta
    alice/protos/data/language
    alice/protos/data/location
    alice/protos/data/tv/tags
    alice/protos/data/scenario/alice_show
    alice/protos/data/scenario/order
    alice/protos/data/scenario/centaur/my_screen
    alice/protos/data/scenario/centaur/teasers
    alice/protos/data/scenario/iot
    alice/protos/data/scenario/music
    alice/protos/data/scenario/reminders
    alice/protos/data/scenario/timer
    alice/protos/data/scenario/video_call
    alice/protos/data/search_result
    alice/protos/data/tv_feature_boarding
    alice/protos/data/video
    alice/protos/div
    alice/protos/endpoint
    alice/protos/endpoint/capabilities/route_manager
    alice/protos/endpoint/events
    alice/protos/extensions
    mapreduce/yt/interface/protos
)

SRCS(
    app_type.proto
    atm.proto
    conditional_action.proto
    content_properties.proto
    data_source_type.proto
    device_state.proto
    directive_channel.proto
    directives_execution_policy.proto
    effect_options.proto
    environment_state.proto
    events.proto
    experiments.proto
    frame.proto
    frame_request_params.proto
    gc_memory_state.proto
    iot.proto
    location.proto
    misspell.proto
    origin.proto
    permissions.proto
    request_params.proto
    response_error_message.proto
    smart_home.proto
    quasar_devices.proto
    subscription_state.proto
    tandem_state.proto
)

IF(NZ_SYNC)
    SRCS(
        forbidden_to_use_alice_protos_in_zenmigration
    )
ENDIF()

END()

RECURSE(
    required_messages
)
