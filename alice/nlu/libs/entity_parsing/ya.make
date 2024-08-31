LIBRARY()

OWNER(
    yorky0
    moath-alali
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/entity_searcher
    alice/nlu/libs/occurrence_searcher
    alice/nlu/proto/entities
    kernel/inflectorlib/phrase/simple
    kernel/lemmer/core
    library/cpp/json
    library/cpp/langs
    library/cpp/logger/global
)

SRCS(
    entity_parsing.cpp
)

END()

RECURSE_FOR_TESTS(ut)
