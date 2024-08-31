PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    alice/protos/data/entity_meta
    alice/protos/data/language
    mapreduce/yt/interface/protos
)

SRCS(
    child_age.proto
    contacts.proto
    contextual_data.proto
    external_entity_description.proto
    fm_radio_info.proto
    lat_lon.proto
    layer.proto
    news_provider.proto
)

END()

RECURSE(
    action
    app_metrika
    channel
    device
    iot
    language
    location
    tv
    tv_feature_boarding
)
