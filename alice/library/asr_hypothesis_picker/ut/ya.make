UNITTEST_FOR(alice/library/asr_hypothesis_picker)

PEERDIR(
    library/cpp/testing/gmock_in_unittest
    alice/library/logger
)

SRCS(
    asr_hypothesis_picker_ut.cpp
    scenarios_detectors_ut.cpp
)

SIZE(MEDIUM)

END()
