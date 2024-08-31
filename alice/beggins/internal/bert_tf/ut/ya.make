GTEST()

SRCS(
    applier_ut.cpp
    preparer_ut.cpp
    tokenizer_ut.cpp
)

DEPENDS(alice/beggins/internal/bert_tf/ut/data)

PEERDIR(
    alice/beggins/internal/bert_tf
    library/cpp/testing/common
    library/cpp/testing/gtest
)

END()
