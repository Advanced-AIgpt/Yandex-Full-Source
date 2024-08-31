LIBRARY()

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/anaphora_resolver/common
    alice/nlu/libs/sample_features
    kernel/lemmer/core
    kernel/lemmer/dictlib
    kernel/lemmer/new_dict/rus
    kernel/lemmer/new_dict/eng
    library/cpp/json
    search/begemot/core
    search/begemot/rules/alice/anaphora_matcher/proto
    search/begemot/rules/alice/session/proto
)

SRCS(
    session_conversion.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
