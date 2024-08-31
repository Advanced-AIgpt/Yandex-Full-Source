PY2MODULE(query_wizard_features_reader)

OWNER(
    movb
)

PYTHON2_ADDINCL()

PEERDIR(
    alice/nlu/libs/query_wizard_features_reader_wrapper
)

BUILDWITH_CYTHON_CPP(query_wizard_features_reader.pyx --module-name query_wizard_features_reader)

ALLOCATOR(LF)

END()
