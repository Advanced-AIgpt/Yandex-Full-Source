LIBRARY()

OWNER(
    igor-darov
    g:alice
)

SRCS(
    bow.cpp
    demo_smart_home.cpp
    entities.cpp
    indexer.cpp
    iot.cpp
    preprocessor.cpp
    priority.cpp
    query_parser.cpp
    scheme.sc
    typed_hypothesis_assembler.cpp
    utils.cpp
)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/nlu/libs/fst
    alice/nlu/libs/request_normalizer
    alice/nlu/libs/token_aligner
    kernel/lemmer/core
    library/cpp/regex/pcre
    library/cpp/resource
    library/cpp/scheme
)

INCLUDE(${ARCADIA_ROOT}/alice/begemot/lib/fst/data/fst_data_archive/resource.make)

FROM_SANDBOX(
    ${FST_ARCHIVE_RESOURCE_ID}
    OUT_NOAUTO
    fst/ru/time/time.fst
    fst/ru/time/flags.txt
    fst/ru/time/sequence.txt
    fst/ru/time/symbols.sym
    fst/ru/datetime/datetime.fst
    fst/ru/datetime/flags.txt
    fst/ru/datetime/sequence.txt
    fst/ru/datetime/symbols.sym
    fst/ru/datetime_range/datetime_range.fst
    fst/ru/datetime_range/flags.txt
    fst/ru/datetime_range/sequence.txt
    fst/ru/datetime_range/symbols.sym
)

RESOURCE(
    fst/ru/time/time.fst                     fst/ru/time/time.fst
    fst/ru/time/flags.txt                    fst/ru/time/flags.txt
    fst/ru/time/sequence.txt                 fst/ru/time/sequence.txt
    fst/ru/time/symbols.sym                  fst/ru/time/symbols.sym
    fst/ru/datetime/datetime.fst             fst/ru/datetime/datetime.fst
    fst/ru/datetime/flags.txt                fst/ru/datetime/flags.txt
    fst/ru/datetime/sequence.txt             fst/ru/datetime/sequence.txt
    fst/ru/datetime/symbols.sym              fst/ru/datetime/symbols.sym
    fst/ru/datetime_range/datetime_range.fst fst/ru/datetime_range/datetime_range.fst
    fst/ru/datetime_range/flags.txt          fst/ru/datetime_range/flags.txt
    fst/ru/datetime_range/sequence.txt       fst/ru/datetime_range/sequence.txt
    fst/ru/datetime_range/symbols.sym        fst/ru/datetime_range/symbols.sym

    data/normalized_names.json              normalized_names.json

    data/rus/demo_sh.json                   rus_demo_sh.json
    data/rus/entities.json                  rus_entities.json
    data/rus/spelling_variations.txt        rus_spelling_variations.txt
    data/rus/subsynonyms.txt                rus_subsynonyms.txt
    data/rus/synonyms.txt                   rus_synonyms.txt
    data/rus/templates.json                 rus_templates.json

    data/ara/demo_sh.json                   ara_demo_sh.json
    data/ara/entities.json                  ara_entities.json
    data/ara/subsynonyms.txt                ara_subsynonyms.txt
    data/ara/synonyms.txt                   ara_synonyms.txt
    data/ara/templates.json                 ara_templates.json
)

END()

RECURSE_FOR_TESTS(ut)
