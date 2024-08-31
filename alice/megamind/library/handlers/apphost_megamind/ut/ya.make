UNITTEST_FOR(alice/megamind/library/handlers/apphost_megamind)

OWNER(g:megamind)

SIZE(MEDIUM)

PEERDIR(
    alice/library/blackbox
    alice/library/client
    alice/library/json
    alice/library/network
    alice/library/proto
    alice/library/scenarios/data_sources
    alice/megamind/library/apphost_request
    alice/megamind/library/testing
    alice/begemot/lib/api/experiments
    library/cpp/resource
    search/session/compression
    apphost/lib/proto_answers
)

SRCS(
    begemot_ut.cpp
    blackbox_ut.cpp
    combinators_ut.cpp
    components_ut.cpp
    fallback_response_ut.cpp
    grpc/grpc_finalize_handler_ut.cpp
    grpc/grpc_setup_handler_ut.cpp
    misspell_ut.cpp
    personal_intents_ut.cpp
    proactivity_ut.cpp
    polyglot_ut.cpp
    postpone_log_writer_ut.cpp
    query_tokens_stats_ut.cpp
    saas_ut.cpp
    skr_ut.cpp
    ut_helper.cpp
    walker_prepare_ut.cpp
    walker_monitoring_ut.cpp
    websearch_ut.cpp
)

DATA(
    sbr://693769852 # geodata5-xurma.bin
    sbr://3351884173 # geodata6.bin 2022-07-22
)

RESOURCE(
    begemot_native_json_response.json /begemot_native_json_response
    begemot_native_proto_response.json /begemot_native_proto_response
    personal_intents_response.data /personal_intents_response
    personal_intents_response.json /personal_intents_response_json
    polyglot_begemot_native_proto_response.json /polyglot_begemot_native_proto_response
    query_stats_response.data /query_stats_response
)

END()
