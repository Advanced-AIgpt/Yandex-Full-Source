LIBRARY()

OWNER(
    g:alice_boltalka
)

PEERDIR(
    contrib/libs/intel/mkl
    alice/boltalka/extsearch/base/factors
    alice/boltalka/extsearch/base/calc_factors
    alice/boltalka/extsearch/base/util
    alice/boltalka/generative/service/proto
    alice/boltalka/libs/factors/proto
    alice/boltalka/libs/text_utils
    alice/boltalka/libs/dssm_model
    alice/nlu/libs/tf_nn_model
    kernel/externalrelev
    kernel/info_request
    kernel/matrixnet
    kernel/searchlog
    kernel/search_daemon_iface
    library/cpp/hnsw/index
    library/cpp/iterator
    library/cpp/langs
    library/cpp/neh
    library/cpp/regex/pcre
    library/cpp/threading/local_executor
    library/cpp/unistat
    search/base
    search/config
    search/rank
    search/reqparam
    search/session
)

SRCS(
    async_logger.cpp
    dssm_index.cpp
    dssm_model_with_indexes.cpp
    factor_dssm_model.cpp
    index_data.cpp
    fixlist.cpp
    knn_index.cpp
    relevance.cpp
    relevance_error_request.cpp
    search.cpp
    static_factors.cpp
    tf_ranker.cpp
    unistat_registry.cpp
    util.cpp
)

GENERATE_ENUM_SERIALIZATION(dssm_index.h)

END()

