LIBRARY(query_wizard_features_reader_wrapper)
EXPORTS_SCRIPT(reader.exports)

OWNER(movb)

SRCS(
    query_wizard_features_reader_wrapper.cpp
)

PEERDIR(
    alice/nlu/query_wizard_features/reader
)

END()
