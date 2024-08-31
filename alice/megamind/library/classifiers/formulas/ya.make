LIBRARY()

OWNER(
    olegator
    g:megamind
)

PEERDIR(
    alice/library/client
    alice/library/experiments
    alice/library/logger
    alice/megamind/library/classifiers/formulas/protos
    alice/megamind/library/config
    alice/megamind/library/config/protos
    alice/megamind/library/experiments
    kernel/formula_storage/shared_formulas_adapter
    kernel/matrixnet
    library/cpp/langs
    library/cpp/protobuf/util
)

SRCS(
    formulas_description.cpp
    formulas_storage.cpp
)

END()

RECURSE_FOR_TESTS(ut)
