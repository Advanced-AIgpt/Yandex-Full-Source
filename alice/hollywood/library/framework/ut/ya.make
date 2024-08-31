UNITTEST_FOR(alice/hollywood/library/framework)

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/framework/core
    alice/hollywood/library/framework/core/codegen
    alice/hollywood/library/framework/proto
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/framework/ut/nlg
    alice/hollywood/library/framework/ut/proto
    alice/hollywood/library/http_proxy
    alice/library/json
    alice/megamind/protos/scenarios
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    framework_ut.cpp
    ut/analitics_info_ut.cpp
    ut/black_box_ut.cpp
    ut/directives_codegen_ut.cpp
    ut/fastdata_ut.cpp
    ut/framework_migration_ut.cpp
    ut/memento_ut.cpp
    ut/renderdata_ut.cpp
    ut/render_ut.cpp
    ut/request_flags_ut.cpp
    ut/response_ut.cpp
    ut/run_features_ut.cpp
    ut/scenario_ut.cpp
    ut/scene_graph_ut.cpp
    ut/semantic_frame_ut.cpp
    ut/self_register_ut.cpp
    ut/setupsource_ut.cpp
    ut/stack_engine_ut.cpp
)

END()
