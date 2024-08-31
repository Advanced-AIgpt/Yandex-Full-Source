#include <alice/cuttlefish/library/experiments/experiments.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/folder/path.h>
#include "common.h"

using namespace NVoice::NExperiments;

const TFsPath TEST_DATA = TFsPath(ArcadiaSourceRoot()) / "alice/cuttlefish/library/experiments/ut/data";


Y_UNIT_TEST_SUITE(SmokeTests) {

    Y_UNIT_TEST(NonexistentExperimentsFile) {
        UNIT_ASSERT_EXCEPTION(
            TExperiments(TExperiments::TConfig{TEST_DATA/"nonexistent.json", TEST_DATA/"macros.json"}),
            yexception
        );
    }

    Y_UNIT_TEST(ValidExperimentsFile)
    {
        UNIT_ASSERT_NO_EXCEPTION(TExperiments(TExperiments::TConfig{
             MakeFile(R"__([
                {
                    "flags": [
                        ["set", ".MyValue", 1]
                    ]
                }
            ])__"),
            TEST_DATA/"macros.json"
        }));

        UNIT_ASSERT_NO_EXCEPTION(TExperiments(TExperiments::TConfig{
             MakeFile(R"__([
                {
                    "share": 0.25,
                    "flags": [
                        ["set", ".MyValue", 1]
                    ]
                }
            ])__"),
            TEST_DATA/"macros.json"
        }));

        UNIT_ASSERT_NO_EXCEPTION(TExperiments(TExperiments::TConfig{
            MakeFile(R"__([
                {
                    "share": 0.49,
                    "flags": [
                        ["set", ".MyValue", 1]
                    ]
                },
                {
                    "share": 0.51,
                    "flags": [
                        ["set", ".MyValue", 2]
                    ]
                }
            ])__"),
            TEST_DATA/"macros.json"
        }));
    }

    Y_UNIT_TEST(InvalidExperimentFiles) {
        // empty experiment
        UNIT_ASSERT_EXCEPTION(TExperiments(TExperiments::TConfig{
            MakeFile(R"__([
                {}
            ])__"),
            TEST_DATA/"macros.json"
        }), yexception);

        // control share isn't supported
        UNIT_ASSERT_EXCEPTION(TExperiments(TExperiments::TConfig{
            MakeFile(R"__([
                {
                    "control_share": 0.01,
                    "flags": [
                        ["set", ".MyValue", 1]
                    ]
                }
            ])__"),
            TEST_DATA/"macros.json"
        }), yexception);

        // share must not be greater than 1.0
        UNIT_ASSERT_EXCEPTION(TExperiments(TExperiments::TConfig{
            MakeFile(R"__([
                {
                    "share": 1.01,
                    "flags": [
                        ["set", ".MyValue", 1]
                    ]
                }
            ])__"),
            TEST_DATA/"macros.json"
        }), yexception);
    }

    Y_UNIT_TEST(Full) {
        const TString experimentsFilename = MakeFile(R"__([
                {
                    "id": "PureLocalExperiment",
                    "flags": [
                        ["set", ".PureLocalExperiment", 1]
                    ]
                }
            ])__");


        TExperiments experiments(TExperiments::TConfig{experimentsFilename, TEST_DATA/"macros.json"});

        const NAliceProtocol::TSessionContext sessionContext = CreateSessionContext({});

        {
            const TEventPatcher eventPatcher = experiments.CreatePatcherForSession(sessionContext, CreateJson(R"__({
                "header": {"namespace": "X", "name": "X"},
                "payload": {}
            })__"));

            NJson::TJsonValue event = CreateJson(R"__({
                "header": {},
                "payload": {}
            })__");

            eventPatcher.Patch(event, sessionContext);
            UNIT_ASSERT_VALUES_EQUAL(event, CreateJson(R"__({
                "header": {},
                "payload": {
                    "PureLocalExperiment": 1,
                    "request": {
                        "experiments": {
                        }
                    }
                }
            })__"));
        }
    }
};
