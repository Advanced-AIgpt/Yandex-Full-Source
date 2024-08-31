PY3_LIBRARY(apply_nlg_dssm)

OWNER(
    nzinov
    g:alice_boltalka
)

#PYTHON3_ADDINCL()
#USE_PYTHON3()

PEERDIR(
    alice/boltalka/libs/dssm_model
)

PY_SRCS(apply_nlg_dssm.pyx)

END()
