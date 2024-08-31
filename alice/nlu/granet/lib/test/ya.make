LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    batch.cpp
    batch_result.cpp
    context_storage.cpp
    dataset.cpp
    dataset_builder.cpp
    dataset_mock_updater.cpp
    dataset_processor.cpp
    dataset_statistics.cpp
    fetcher.cpp
    metrics.cpp
    sample_creator.cpp
    sample_processor.cpp
    table_formatter.cpp
    tsv.cpp
)

PEERDIR(
    alice/begemot/lib/api/params
    alice/begemot/lib/locale
    alice/nlu/granet/lib/compiler
    alice/nlu/granet/lib/grammar
    alice/nlu/granet/lib/parser
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/user_entity
    alice/nlu/granet/lib/utils
    alice/nlu/libs/request_normalizer
    dict/dictutil
    dict/nerutil
    library/cpp/containers/comptrie
    library/cpp/cgiparam
    library/cpp/http/client
    library/cpp/http/simple
    library/cpp/string_utils/url
    library/cpp/iterator
    library/cpp/json
    library/cpp/map_text_file
    library/cpp/packers
    library/cpp/testing/common
)

END()
