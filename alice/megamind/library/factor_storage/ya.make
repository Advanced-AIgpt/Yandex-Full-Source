LIBRARY()

OWNER(g:megamind)

PEERDIR(
    kernel/factor_slices
    kernel/factor_storage

    # these libraries contain factor codegen, we must reference them
    # for CreateFactorDomain to work
    kernel/alice/asr_factors_info
    kernel/alice/begemot_nlu_factors_info/fill_factors
    kernel/alice/begemot_query_factors_info
    kernel/alice/device_state_factors_info
    kernel/alice/direct_scenario_factors_info/fill_factors
    kernel/alice/gc_scenario_factors_info/fill_factors
    kernel/alice/music_scenario_factors_info/fill_factors
    kernel/alice/query_tokens_factors_info
    kernel/alice/search_scenario_factors_info/fill_factors
    kernel/alice/video_scenario/fill_factors
    kernel/begemot_query_factors_info/fill_factors
    search/web/blender/factors_info
)

SRCS(
    factor_storage.cpp
)

END()
