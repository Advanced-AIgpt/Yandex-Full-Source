PY2MODULE(apply_nlg_dssm)

OWNER(
    nzinov
    g:alice_boltalka
)

PYTHON2_ADDINCL()

PEERDIR(
    alice/boltalka/libs/dssm_model
)

COPY(
    apply_nlg_dssm.pyx
    FROM alice/boltalka/py_libs/apply_nlg_dssm
)

BUILDWITH_CYTHON_CPP(apply_nlg_dssm.pyx --module-name apply_nlg_dssm)

END()
