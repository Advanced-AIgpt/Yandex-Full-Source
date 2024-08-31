LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
    g:begemot
)

PEERDIR(
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/utils
    alice/nlu/libs/token_aligner
    kernel/lemmer/core
    library/cpp/scheme
    search/begemot/apphost
    search/begemot/apphost/protos
    search/begemot/core
    search/begemot/core/proto
    search/begemot/rules/alice/ar_fst/proto
    search/begemot/rules/alice/custom_entities/proto
    search/begemot/rules/alice/nonsense_tagger/proto
    search/begemot/rules/alice/request/proto
    search/begemot/rules/alice/type_parser/proto
    search/begemot/rules/alice/thesaurus/proto
    search/begemot/rules/alice/translit/proto
    search/begemot/rules/alice/user_entities/proto
    search/begemot/rules/entity_finder/proto
    search/begemot/rules/external_markup/proto
    search/begemot/rules/fst/proto
    search/begemot/rules/granet/proto
    search/begemot/rules/occurrences/custom_entities/rule/proto
    search/begemot/rules/text/proto
)

SRCS(
    aligned_entities.cpp
    date.cpp
    entities_collector.cpp
    entity_collecting.cpp
    entity_finder.cpp
    entity_to_proto.cpp
    fio.cpp
    geo.cpp
    workaround.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
