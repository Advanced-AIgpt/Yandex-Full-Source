LIBRARY()

OWNER(
    deemonasd
    g:hollywood
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/generative/service/proto
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/general_conversation/common
    alice/hollywood/library/scenarios/general_conversation/containers
    alice/hollywood/library/scenarios/general_conversation/proto
    alice/hollywood/library/scenarios/search/utils
    alice/hollywood/protos
    alice/megamind/protos/analytics/scenarios/general_conversation
    alice/megamind/protos/common
    alice/library/experiments
    alice/library/json
    alice/library/scled_animations
    alice/library/util
    alice/nlu/libs/binary_classifier

    kernel/inflectorlib/phrase/simple
    contrib/libs/re2

    library/cpp/iterator
    library/cpp/string_utils/quote
    library/cpp/timezone_conversion
)

SRCS(
    aggregated_candidates.cpp
    bert_reranker.cpp
    context_wrapper.cpp
    entity_candidates.cpp
    filter_candidates.cpp
    filter_by_embedding_model.cpp
    generative_tale_utils.cpp
    phead_scorer.cpp
    proactivity_candidates.cpp
    search_candidates.cpp
    search_params.cpp
    seq2seq_candidates.cpp
    render_utils.cpp
    utils.cpp
)

END()
