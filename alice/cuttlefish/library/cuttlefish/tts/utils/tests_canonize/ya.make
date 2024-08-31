EXECTEST()

OWNER(g:voicetech-infra)

DEPENDS(
    alice/cuttlefish/library/cuttlefish/tts/utils/ut
)

RUN(
    NAME TCuttlefishTtsUtilsCanonizeTest.TestGetVoiceList
    alice-cuttlefish-library-cuttlefish-tts-utils-ut TCuttlefishTtsUtilsCanonizeTest::TestGetVoiceList
    STDOUT ${TEST_OUT_ROOT}/TCuttlefishTtsUtilsCanonizeTest.TestGetVoiceList.stdout
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/TCuttlefishTtsUtilsCanonizeTest.TestGetVoiceList.stdout
)

RUN(
    NAME TCuttlefishTtsUtilsCanonizeTest.TestGetTtsBackendRequestItemTypeForLang
    alice-cuttlefish-library-cuttlefish-tts-utils-ut TCuttlefishTtsUtilsCanonizeTest::TestGetTtsBackendRequestItemTypeForLang
    STDOUT ${TEST_OUT_ROOT}/TCuttlefishTtsUtilsCanonizeTest.TestGetTtsBackendRequestItemTypeForLang.stdout
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/TCuttlefishTtsUtilsCanonizeTest.TestGetTtsBackendRequestItemTypeForLang.stdout
)

RUN(
    NAME TCuttlefishTtsUtilsCanonizeTest.TestGetSpeakersJson
    alice-cuttlefish-library-cuttlefish-tts-utils-ut TCuttlefishTtsUtilsCanonizeTest::TestGetSpeakersJson
    STDOUT ${TEST_OUT_ROOT}/TCuttlefishTtsUtilsCanonizeTest.TestGetSpeakersJson.stdout
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/TCuttlefishTtsUtilsCanonizeTest.TestGetSpeakersJson.stdout
)

RUN(
    NAME TCuttlefishTtsUtilsCanonizeTest.TestGetSpeakersJsonPythonUniproxyFormat
    alice-cuttlefish-library-cuttlefish-tts-utils-ut TCuttlefishTtsUtilsCanonizeTest::TestGetSpeakersJsonPythonUniproxyFormat
    STDOUT ${TEST_OUT_ROOT}/TCuttlefishTtsUtilsCanonizeTest.TestGetSpeakersJsonPythonUniproxyFormat.stdout
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/TCuttlefishTtsUtilsCanonizeTest.TestGetSpeakersJsonPythonUniproxyFormat.stdout
)

END()
