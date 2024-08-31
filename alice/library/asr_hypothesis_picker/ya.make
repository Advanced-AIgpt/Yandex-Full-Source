LIBRARY()

OWNER(
    igor-darov
    g:alice
)

PEERDIR(
    alice/library/logger
    alice/library/client
)

SRCS(
    asr_hypothesis_picker.cpp
    scenarios_detectors.cpp
)

END()

RECURSE_FOR_TESTS(ut)
