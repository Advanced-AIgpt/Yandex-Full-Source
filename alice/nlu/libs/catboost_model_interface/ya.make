LIBRARY(catboost_model_interface)
EXPORTS_SCRIPT(calcer.exports)

OWNER(alipov)

SRCS(
    model_calcer_wrapper.cpp
)

PEERDIR(
    catboost/libs/cat_feature
    catboost/libs/model
)

END()
