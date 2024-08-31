LIBRARY()

OWNER(
    alipov
    g:alice_boltalka
)

PEERDIR(
    kernel/externalrelev
    kernel/generated_factors_info
)

SRCS(
    factor_names.cpp
    ${BINDIR}/factors_gen.cpp
)

SPLIT_CODEGEN(kernel/generated_factors_info/factors_codegen factors_gen NNlg)

END()
