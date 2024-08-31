PY2_LIBRARY()

OWNER(
    akastornov
    movb
)

PEERDIR(alice/nlu/libs/query_wizard_features_reader_wrapper)

PY_SRCS(alice/nlu/py_libs/query_wizard_features_reader/query_wizard_features_reader.pyx=query_wizard_features_reader)

END()
