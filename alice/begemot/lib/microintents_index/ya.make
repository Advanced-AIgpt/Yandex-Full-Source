LIBRARY()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/boltalka/libs/text_utils
    contrib/libs/yaml-cpp
    kernel/dssm_applier/nn_applier/lib
    library/cpp/dot_product
    library/cpp/langs
    library/cpp/yaml/as
)

SRCS(
    microintents_index.cpp
)

END()
