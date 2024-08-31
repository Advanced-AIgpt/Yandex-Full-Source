PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

PEERDIR(
    alice/library/client/protos
    alice/megamind/library/session/protos
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    apphost/lib/proto_answers
)

SRCS(
    analytics_logs_context.proto
    begemot_response_parts.proto
    client.proto
    combinators.proto
    error.proto
    fake_item.proto
    preclassify.proto
    request_meta.proto
    required_node_meta.proto
    scenario.proto
    scenario_errors.proto
    scenarios_text_response.proto
    stage_timestamp.proto
    uniproxy_request.proto
    web_search_query.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
