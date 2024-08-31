PY2_LIBRARY()

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    alice/nlu/verstehen/config
    alice/nlu/verstehen/granet_errors
    alice/nlu/verstehen/preprocess
    alice/nlu/verstehen/util
    # 3rd party
    alice/nlu/py_libs/granet
    catboost/python-package/lib
    contrib/python/numpy
    contrib/python/scikit-learn
    contrib/python/Werkzeug
    library/python/hnsw/lib
)

PY_SRCS(
    NAMESPACE verstehen.index
    __init__.py
    index.py
    index_registry.py
    catboost/__init__.py
    catboost/catboost_index.py
    catboost/catboost_reranker.py
    composite/__init__.py
    composite/composite_index.py
    embedding/knn/__init__.py
    embedding/knn/hnsw_index.py
    embedding/knn/knn_index.py
    embedding/__init__.py
    embedding/dssm_index.py
    embedding/logreg_dssm_index.py
    embedding/mlp_dssm_index.py
    granet/__init__.py
    granet/granet_index.py
    word_match/__init__.py
    word_match/bm25_index.py
)

END()
