LIBRARY()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    basic_text_factors.cpp
    dssm_cos_factors.cpp
    factor_calcer.cpp
    nlg_search_factor_calcer.cpp
    pronoun_factors.cpp
    rus_lister_factors.cpp
    text_intersect_factors.cpp
    is_dssm_index_factor.cpp
    is_knn_index_factor.cpp
    informativeness_factor.cpp
    seq2seq_factor.cpp
)

PEERDIR(
    alice/boltalka/extsearch/base/factors
    library/cpp/containers/dense_hash
    library/cpp/dot_product
    util
)


END()

