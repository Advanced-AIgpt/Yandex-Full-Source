#include <alice/cuttlefish/library/experiments/ut/common.h>
#include <alice/cuttlefish/library/experiments/experiment_patch.h>
#include <alice/cuttlefish/library/experiments/patch_functions.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>
#include <util/folder/path.h>

using namespace NVoice::NExperiments;


Y_UNIT_TEST_SUITE(PatchFunctions) {
    Y_UNIT_TEST_SUITE(Common) {
        Y_UNIT_TEST(InvalidJson) {
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ {} ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ [] ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ null ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ 18 ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "nonexisting-command", "", "" ])__"), yexception);
        }

        Y_UNIT_TEST(EmptyJson) {
            // it seems weird but still
            UNIT_ASSERT_NO_EXCEPTION(CreateExperimentPatch(R"__([])__"));
        }
    } // Y_UNIT_TEST_SUITE(Common)

    Y_UNIT_TEST_SUITE(Set) {
        Y_UNIT_TEST(TooFewArguments) {
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__(["set"])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__(["set", ".MyField"])__"), yexception);
        }

        Y_UNIT_TEST(ReplacePaylod) {
            auto result = CreateAndPatch(
                R"__(["set", "", {"MyPayload": []}])__",
                R"__({"header": { }, "payload": { }})__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": {"MyPayload": []}
            })__"));
        }

        Y_UNIT_TEST(AnonymousField) {
            auto result = CreateAndPatch(
                R"__(["set", ".", {"MyPayload": []}])__",
                R"__({"header": { }, "payload": { }})__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": {
                    "": {"MyPayload": []}
                }
            })__"));
        }

        Y_UNIT_TEST(CreateTargetNode) {
            auto result = CreateAndPatch(
                R"__(["set", ".MyField", "MyValue"])__",
                R"__({"header": { }, "payload": { }})__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": {"MyField": "MyValue"}
            })__"));
        }

        Y_UNIT_TEST(CreateTargetSubNodes) {
            auto result = CreateAndPatch(
                R"__(["set", ".long.path.to.my.field", ["my", "list", "value"]])__",
                R"__({"header": { }, "payload": { }})__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": {
                    "long": {"path": {"to": {"my": {"field": ["my", "list", "value"]}}}}
                }
            })__"));
        }

        Y_UNIT_TEST(ChangeTargetNode1) {
            auto result = CreateAndPatch(
                R"__(["set", ".TargetField", "TargetValue"])__",
                R"__({
                    "header": { },
                    "payload": {
                        "TargetField": [1, 2, 3],
                        "AnotherField": { }
                    }
                })__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": {
                    "TargetField": "TargetValue",
                    "AnotherField": { }
                }
            })__"));
        }

        Y_UNIT_TEST(ChangeTargetNode2) {
            auto result = CreateAndPatch(
                R"__(["set", ".request.experiments.MyFlag", {"x": "X"}])__",
                R"__({
                    "header": { },
                    "payload": {
                        "request": {
                            "some": "thing",
                            "experiments": ["MyFlag"]
                        }
                    }
                })__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                    "payload": {
                        "request": {
                            "some": "thing",
                            "experiments": {
                                "MyFlag": {"x": "X"}
                            }
                        }
                    }
            })__"));
        }

    }; // Y_UNIT_TEST_SUITE(Set)

    Y_UNIT_TEST_SUITE(SetIfNone) {
        Y_UNIT_TEST(SetPayload) {
            auto result = CreateAndPatch(
                R"__(["set_if_none", "", {"MyPayload": []}])__",
                R"__({"header": { }})__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": {"MyPayload": []}
            })__"));
        }

        Y_UNIT_TEST(DontSetPayload) {
            auto result = CreateAndPatch(
                R"__(["set_if_none", "", {"MyPayload": []}])__",
                R"__({"header": { }, "payload": { }})__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": { }
            })__"));
        }

        Y_UNIT_TEST(CreateTargetNode) {
            auto result = CreateAndPatch(
                R"__(["set_if_none", ".MyField", "MyValue"])__",
                R"__({"header": { }, "payload": { }})__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": {"MyField": "MyValue"}
            })__"));
        }

        Y_UNIT_TEST(DontCreateTargetNode) {
            auto result = CreateAndPatch(
                R"__(["set_if_none", ".MyField", "MyValue"])__",
                R"__({"header": { }, "payload": { "MyField": 14 }})__"
            );
            UNIT_ASSERT(result.first);
            UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                "header": { },
                "payload": { "MyField": 14 }
            })__"));
        }

    }; // Y_UNIT_TEST_SUITE(SetIfNone)

     Y_UNIT_TEST_SUITE(Append) {
        Y_UNIT_TEST(TooFewArguments) {
            UNIT_ASSERT_EXCEPTION(
                CreateExperimentPatch(R"__(["append", ".MyField"])__"),
                yexception
            );
        }

        Y_UNIT_TEST(ValueForTrickyCase) {
            // For `.request.experiments` path value must be string
            UNIT_ASSERT_EXCEPTION(
                CreateExperimentPatch(R"__(["append", ".request.experiments", 0])__"),
                yexception
            );
            UNIT_ASSERT_EXCEPTION(
                CreateExperimentPatch(R"__(["append", ".request.experiments", []])__"),
                yexception
            );
            UNIT_ASSERT_EXCEPTION(
                CreateExperimentPatch(R"__(["append", ".request.experiments", {}])__"),
                yexception
            );
            UNIT_ASSERT_NO_EXCEPTION(
                CreateExperimentPatch(R"__(["append", ".request.experiments", "x"])__")
            );
        }

        Y_UNIT_TEST(CreateAndAppend) {
            {
                auto result = CreateAndPatch(
                    R"__(["append", ".MyField", "MyValue"])__",
                    R"__({"header": { }, "payload": { }})__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": { "MyField": ["MyValue"] }
                })__"));
            } {
                auto result = CreateAndPatch(
                    R"__([ "append", ".MyField", [[{"MyValue": 48}]] ])__",
                    R"__({"header": { }, "payload": { }})__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": {
                        "MyField": [
                            [[{"MyValue": 48}]]
                        ]
                    }
                })__"));
            } {
                auto result = CreateAndPatch(
                    R"__([ "append", ".My.Field", {} ])__",
                    R"__({"header": { }, "payload": { }})__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": {
                        "My": {
                            "Field": [
                                {}
                            ]
                        }
                    }
                })__"));
            }
        }

        Y_UNIT_TEST(AppendIntoExisting) {
            {
                auto result = CreateAndPatch(
                    R"__([ "append", ".MyField", "MyValue" ])__",
                    R"__({ "header": { }, "payload": { "MyField": [] } })__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": { "MyField": ["MyValue"] }
                })__"));
            } {
                auto result = CreateAndPatch(
                    R"__([ "append", ".MyField", [[{"MyValue": 48}]] ])__",
                    R"__({ "header": { }, "payload": { "MyField": [1, 2] } })__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": {
                        "MyField": [
                            1, 2, [[{"MyValue": 48}]]
                        ]
                    }
                })__"));
            }
        }

        Y_UNIT_TEST(DontAppendIfExist) {
            {
                auto result = CreateAndPatch(
                    R"__([ "append", ".MyField", 1 ])__",
                    R"__({ "header": { }, "payload": { "MyField": [1] } })__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": { "MyField": [1] }
                })__"));
            } {
                auto result = CreateAndPatch(
                    R"__([ "append", ".MyField", [["X", {}, 16]] ])__",
                    R"__({
                        "header": { },
                        "payload": {
                            "MyField": [
                                [["X", {}, 16]]
                            ]
                        }
                    })__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": {
                        "MyField": [
                            [["X", {}, 16]]
                        ]
                    }
                })__"));
            }
        }

        Y_UNIT_TEST(Tricky) {
            {
                auto result = CreateAndPatch(
                    R"__([ "append", ".request.experiments", "MyField" ])__",
                    R"__({ "header": { }, "payload": { } })__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": {
                        "request": {
                            "experiments": {
                                "MyField": "1"
                            }
                        }
                    }
                })__"));
            } {
                // Existing value must not be overwritten
                auto result = CreateAndPatch(
                    R"__([ "append", ".request.experiments", "MyField" ])__",
                    R"__({
                        "header": { },
                        "payload": {
                            "request": {
                                "experiments": {
                                    "MyField": "48"
                                }
                            }
                        }
                    })__"
                );
                UNIT_ASSERT(result.first);
                UNIT_ASSERT_VALUES_EQUAL(result.second, CreateJson(R"__({
                    "header": { },
                    "payload": {
                        "request": {
                            "experiments": {
                                "MyField": "48"
                            }
                        }
                    }
                })__"));
            }
        }

     }; // Y_UNIT_TEST_SUITE(Append)

     Y_UNIT_TEST_SUITE(Extend) {
        Y_UNIT_TEST(AllowedValues) {
            // too few arguments
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend" ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".MyField" ])__"), yexception);

            // -- regular extend
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".MyField", 0 ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".MyField", {"x": 1} ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".MyField", "x" ])__"), yexception);
            // value must be Array of anything
            UNIT_ASSERT_NO_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".MyField", [] ])__"));
            UNIT_ASSERT_NO_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".MyField", [{}] ])__"));

            // -- request.experiments extend
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".request.experiments", 0 ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".request.experiments", "" ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".request.experiments", [ 1 ] ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".request.experiments", [ [] ] ])__"), yexception);
            UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".request.experiments", [ {} ] ])__"), yexception);
            // value must be Map or Array of strings
            UNIT_ASSERT_NO_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".request.experiments", { } ])__"));
            UNIT_ASSERT_NO_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".request.experiments", [  ] ])__"));
            UNIT_ASSERT_NO_EXCEPTION(CreateExperimentPatch(R"__([ "extend", ".request.experiments", [ "" ] ])__"));
        }

        Y_UNIT_TEST(CreateAndExtend) {
            CheckPatch(
                // PATCH
                R"__([ "extend", ".My.Field", [ 1, 2, {} ] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": { }
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {
                        "My": {
                            "Field": [ 1, 2, {} ]
                        }
                    }
                })__");
            CheckPatch(
                // PATCH
                R"__([ "extend", ".My.Field", [] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": { }
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {
                        "My": {
                            "Field": []
                        }
                    }
                })__");
        }

        Y_UNIT_TEST(ExtendExisting) {
            CheckPatch(
                // PATCH
                R"__([ "extend", ".My.Field", [ {}, 1, 2, {"x": 14}, 3, [] ] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": {
                        "My": {
                            "Field": [ 1, {"x": 15}, 3, {}, 5 ]
                        }
                    }
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {
                        "My": {
                            "Field": [ 1, {"x": 15}, 3, {}, 5, 2, {"x": 14}, [] ]
                        }
                    }
                })__");
        }

        Y_UNIT_TEST(DontChangeDstType) {
            CheckPatch(
                // PATCH
                R"__([ "extend", ".My.Field", [ 1, 2 ] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": {
                        "My": {
                            "Field": "STRING"
                        }
                    }
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {
                        "My": {
                            "Field": "STRING"
                        }
                    }
                })__");
        }

        Y_UNIT_TEST(TrickyCreateAndExtend) {
            CheckPatch(
                // PATCH
                R"__([ "extend", ".request.experiments", [ ] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": { }
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {"request": {"experiments": { }}}
                })__");
            CheckPatch(
                // PATCH
                R"__([ "extend", ".request.experiments", [ "X", "Y" ] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": { }
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {"request": {"experiments": { "X": "1", "Y": "1" }}}
                })__");
        }

        Y_UNIT_TEST(TrickyChangeType) {
            CheckPatch(
                // PATCH
                R"__([ "extend", ".request.experiments", [ ] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": {"request": {"experiments": "STRING"}}
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {"request": {"experiments": { }}}
                })__");
            CheckPatch(
                // PATCH
                R"__([ "extend", ".request.experiments", [ "X", "Y" ] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": {"request": {"experiments": "STRING"}}
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {"request": {"experiments": {"X": "1", "Y": "1"}}}
                })__");
        }

        Y_UNIT_TEST(TrickyExtendExisting) {
            CheckPatch(
                // PATCH
                R"__([ "extend", ".request.experiments", ["X", "Z", "Y", "K"] ])__",
                // ORIGINAL EVENT
                R"__({
                    "header": { },
                    "payload": {"request": {"experiments": {"X": 14, "Y": 15}}}
                })__",
                // PATCHED EVENT
                R"__({
                    "header": { },
                    "payload": {"request": {"experiments": {"X": 14, "Y": 15, "Z": "1", "K": "1"}}}
                })__"
            );
        }

    }; // Y_UNIT_TEST_SUITE(Extend)

     Y_UNIT_TEST_SUITE(UaasTestsFromMMCookie) {

        Y_UNIT_TEST(Success) {
            NJson::TJsonValue request;
            request["megamind_cookies"] = "{\"uaas_tests\":[384379]}";
            UNIT_ASSERT(TransferUaasTestsFromMegamindCookie(request));
            UNIT_ASSERT_EQUAL(request["uaas_tests"][0].GetIntegerSafe(), 384379);
        }

        Y_UNIT_TEST(Empty) {
            NJson::TJsonValue request;
            request["megamind_cookies"] = "{\"uaas_tests\":[]}";
            UNIT_ASSERT(!TransferUaasTestsFromMegamindCookie(request));
            UNIT_ASSERT(!request.GetMapSafe().contains("uaas_tests"));
        }

        Y_UNIT_TEST(NotFound) {
            NJson::TJsonValue request;
            UNIT_ASSERT(!TransferUaasTestsFromMegamindCookie(request));
            UNIT_ASSERT(!request.GetMap().contains("uaas_tests"));
        }

    }; // Y_UNIT_TEST_SUITE(UaasTestsFromMMCookie)

}; // Y_UNIT_TEST_SUITE(PatchFunctions)
