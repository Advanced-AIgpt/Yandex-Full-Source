UNITTEST_FOR(alice/megamind/library/classifiers/features)

OWNER(
    olegator
    g:megamind
)

PEERDIR(
    alice/megamind/library/factor_storage
    alice/megamind/library/search
    alice/library/response_similarity
    kernel/factor_storage
    library/cpp/langs
    library/cpp/testing/unittest
)

RESOURCE(
    device_state.json device_state.json
    search_response.json search_response.json
)

SIZE(SMALL)

SRCS(
    calcers_ut.cpp
    current_screen_features_ut.cpp
)

END()
