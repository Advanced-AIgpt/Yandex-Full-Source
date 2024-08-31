UNITTEST_FOR(alice/nlu/libs/iot)

OWNER(
    igor-darov
    g:alice_quality
)

SIZE(MEDIUM)

SRCS(
    custom_entities_ut.cpp
)

PEERDIR(
    alice/library/iot
    alice/nlu/libs/entity_searcher
    library/cpp/resource
    kernel/lemmer/core
    alice/megamind/protos/common
    alice/nlu/libs/request_normalizer
    alice/nlu/granet/lib/sample
    alice/nlu/libs/tokenization
)

RESOURCE(
    iot_configs.json iot_configs.json
    tests.json tests.json
)

END()
