PY2MODULE(nlgsearch)

OWNER(
    nzinov
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/libs/nlgsearch_simple
)

PYTHON2_ADDINCL()

BUILDWITH_CYTHON_CPP(nlgsearch_simple.pyx --module-name nlgsearch)

ALLOCATOR(LF)

END()
