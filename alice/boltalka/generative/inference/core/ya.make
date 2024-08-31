LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
    g:zeliboba
)

IF (AUTOCHECK OR CUDA_VERSION == "11.4" AND CUDNN_VERSION == "8.0.5")
ELSE ()
    MESSAGE(FATAL_ERROR "Please build with -DCUDA_VERSION=11.4 -DCUDNN_VERSION=8.0.5: https://st.yandex-team.ru/DEVTOOLSSUPPORT-15848")
ENDIF()

SRCS(
    data.cpp
    external_info.cpp
    model.cpp
    postprocessing.cpp
    tokenizer.cpp
    util.cpp
)

PEERDIR(
    alice/boltalka/extsearch/base/util
    alice/boltalka/generative/inference/core/proto
    alice/boltalka/libs/text_utils
    dict/mt/libs/nn_base
    dict/mt/libs/nn
    dict/mt/libs/nn/ynmt/encdec
    dict/mt/libs/nn/ynmt_backend/cpu
    dict/mt/libs/nn/ynmt_backend/gpu_if_supported
    dict/mt/libs/filters
    dict/mt/libs/libmt
    kernel/lemmer/core
    ml/zeliboba/libs/sentencepiece/cpp
    contrib/libs/re2
    library/cpp/langs
)

IF(HAVE_CUDA)
    PEERDIR(
        dict/mt/libs/nn/ynmt_backend/gpu
    )
    CFLAGS(-DHAVE_CUDA)
ENDIF()

END()

RECURSE(
    proto
    test
)
