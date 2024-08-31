OWNER(g:alice)

RECURSE(
    analytics
    asr_hypothesis_picker
    billing
    biometry
    blackbox
    calendar_parser
    censor
    client
    compression
    contacts
    datetime
    device_state
    field_differ
    frame
    geo
    geo_resolver
    go
    intent_stats
    iot
    java
    logger
    metrics
    multiroom
    network
    parsed_user_phrase
    proactivity
    proto
    proto_eval
    python
    response_similarity
    restriction_level
    scenarios
    scled_animations
    search_result_parser 
    skill_discovery
    smallgeo
    special_location
    sys_datetime
    tool_log
    typed_frame
    unittest
    url_builder
    util
    version
    versioning
    websearch
    yt
)

RECURSE_FOR_TESTS(
    billing/ut
    biometry/ut
    equalizer/ut
    music/ut
    search_result_parser/ut
    video_common/ut
)