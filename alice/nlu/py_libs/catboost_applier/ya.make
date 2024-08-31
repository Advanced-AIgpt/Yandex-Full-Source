PY2MODULE(catboost_applier)

OWNER(
    alipov
)

PYTHON2_ADDINCL()

PEERDIR(
    alice/nlu/libs/catboost_model_interface
    catboost/libs/cat_feature
    util
)

BUILDWITH_CYTHON_CPP(
    catboost_applier.pyx
    --module-name catboost_applier
)

ALLOCATOR(LF)

END()
