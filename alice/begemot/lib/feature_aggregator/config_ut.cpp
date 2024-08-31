#include <alice/begemot/lib/feature_aggregator/config.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NFeatureAggregator;

Y_UNIT_TEST_SUITE(ValidConfigs) {

    Y_UNIT_TEST(OK) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "EntityFeature"
                    Rules: [
                        {
                            Entity {
                                EntityType: "sys.date"
                            }
                            Experiments: [
                                "EntityFeature_E1"
                            ]
                        },
                        {
                            Entity {
                                EntityType: "sys.date_1"
                            }
                            Experiments: [
                                "EntityFeature_E2", "EntityFeature_E3"
                            ]
                        },
                        {
                            GranetFrame: {
                                FrameName: "e"
                            }
                            Experiments: [
                                "EntityFeature_E4"
                            ]
                        },
                        {
                            Entity {
                                EntityType: "sys.date_1"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "ClassifierFeature"
                    Rules: [
                        {
                            MultiIntentClassifier: {
                                Classifier: "SCENARIOS_WORD_LSTM"
                                IntentName: "ClassifierFeature_intent"
                                UseRawValue: true
                            }
                            Experiments: [
                                "ClassifierFeature_E1"
                            ]
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                IntervalMapping: {
                                    ValueBefore: 0.0
                                    Points: [
                                        {
                                            Threshold: 0.5
                                            ValueAfter: 1.0
                                        }
                                    ]
                                }
                            }
                            Experiments: [
                                "ClassifierFeature_E2"
                            ]
                        },
                        {
                            MultiIntentClassifier: {
                                Classifier: "TOLOKA_WORD_LSTM"
                                IntentName: "ClassifierFeature_intent"
                                UseRawValue: true
                            }
                        }
                    ]
                },
                {
                    Index: 3
                    Name: "DisabledWizDetectionFeature"
                    IsDisabled: true
                    Rules: [
                        {
                            WizDetection: {
                                FeatureName: "shinyserp_politota"
                            }
                        }
                    ]
                }
        ]
        )";

        UNIT_ASSERT_NO_EXCEPTION(ReadConfigFromProtoTxtString(cfg));
    }

    Y_UNIT_TEST(EmptyRulesDisabled) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    IsDisabled: true
                    Rules: []
                }
            ]
        )";

        UNIT_ASSERT_NO_EXCEPTION(ReadConfigFromProtoTxtString(cfg));
    }

    Y_UNIT_TEST(TwoRulesPartialExperiment) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                            Experiments: [
                                "E1", "E2"
                            ]
                        },
                        {
                            MultiIntentClassifier: {
                                Classifier: "TOLOKA_WORD_LSTM"
                                IntentName: "Feature_intent"
                                UseRawValue: true
                            }
                            Experiments: [
                                "E1"
                            ]
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_NO_EXCEPTION(ReadConfigFromProtoTxtString(cfg));
    }

    Y_UNIT_TEST(PrioritiesExample) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                            Experiments: ["E1", "E2"]
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                            Experiments: ["E1"]
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                            Experiments: ["E2"]
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM,
                                UseRawValue: true
                            }
                            Experiments: ["E3"]
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM,
                                UseRawValue: true
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_NO_EXCEPTION(ReadConfigFromProtoTxtString(cfg));
    }
}

Y_UNIT_TEST_SUITE(InvalidConfigs) {

    Y_UNIT_TEST(EmptyName) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: ""
                    Rules: [
                        {
                            Entity {
                                EntityType: "e"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadName) {
            TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature 1"
                    Rules: [
                        {
                            Entity {
                                EntityType: "e"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadReservedName) {
        TString cfg = TString::Join(R"(
            Features: [
                {
                    Index: 1
                    Name: ")", RESERVED_FEATURE_NAME, R"("
                    Rules: [
                        {
                            Entity {
                                EntityType: "e"
                            }
                        }
                    ]
                }
            ]
        )");

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadSameNames) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            Entity {
                                EntityType: "e"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "Feature"
                    Rules: [
                        {
                            Entity {
                                EntityType: "Artist"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadSameNamesDisabled) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            Entity {
                                EntityType: "e"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "Feature"
                    IsDisabled: true
                    Rules: [
                        {
                            Entity {
                                EntityType: "Currency"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadSameIndexes) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature1"
                    Rules: [
                        {
                            Entity {
                                EntityType: "e"
                            }
                        }
                    ]
                },
                {
                    Index: 1
                    Name: "Feature2"
                    Rules: [
                        {
                            Entity {
                                EntityType: "site"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadSameIndexesDisabled) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature1"
                    Rules: [
                        {
                            Entity {
                                EntityType: "e"
                            }
                        }
                    ]
                },
                {
                    Index: 1
                    Name: "Feature2"
                    IsDisabled: true
                    Rules: [
                        {
                            Entity {
                                EntityType: "Swear"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadIndexesNotSorted) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature1"
                    Rules: [
                        {
                            Entity {
                                EntityType: "e"
                            }
                        }
                    ]
                },
                {
                    Index: 3
                    Name: "Feature3"
                    Rules: [
                        {
                            Entity {
                                EntityType: "calc"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "Feature2"
                    Rules: [
                        {
                            Entity {
                                EntityType: "fio"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadEmptyRules) {
            TStringBuf cfg = R"(
                Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: []
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadTwoDefaultRules) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                        },
                        {
                            MultiIntentClassifier: {
                                Classifier: "TOLOKA_WORD_LSTM"
                                IntentName: "Feature_intent"
                                UseRawValue: true
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadTwoRulesSameExperiment) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                            Experiments: [
                                "E1", "E2"
                            ]
                        },
                        {
                            MultiIntentClassifier: {
                                Classifier: "TOLOKA_WORD_LSTM"
                                IntentName: "Feature_intent"
                                UseRawValue: true
                            }
                            Experiments: [
                                "E2", "E1"
                            ]
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadUseRawValueSetButFalse) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: false
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadSameThresholds) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                IntervalMapping: {
                                    ValueBefore: 0.0
                                    Points: [
                                        {
                                            Threshold: 0.1
                                            ValueAfter: 1.0
                                        },
                                        {
                                            Threshold: 0.1
                                            ValueAfter: 2.0
                                        }
                                    ]
                                }
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadUnsortedThresholds) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                IntervalMapping: {
                                    ValueBefore: 0.0
                                    Points: [
                                        {
                                            Threshold: 0.3
                                            ValueAfter: 1.0
                                        },
                                        {
                                            Threshold: 0.2
                                            ValueAfter: 2.0
                                        }
                                    ]
                                }
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadPrioritiesMainRuleExperiments) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                            Experiments: ["E1"]
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadPrioritiesExperiments) {
        TStringBuf cfg = R"(
            Features: [
                {
                    Index: 1
                    Name: "Feature"
                    Rules: [
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                            Experiments: ["E1"]
                        },
                        {
                            BinaryClassifier: {
                                Classifier: ALICE_GC_DSSM
                                UseRawValue: true
                            }
                            Experiments: ["E1", "E2"]
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(cfg), TValidationException);
    }

    Y_UNIT_TEST(BadSameNamesWithUnkownRules) {
        auto freshConfigStr = R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.NO_THING_LIKE_THIS"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.NO_THING_LIKE_THIS"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(freshConfigStr, /* ignoreUnknownRules */ true), TValidationException);
    }

    Y_UNIT_TEST(BadSameIndexesWithUnkownRules) {
        auto freshConfigStr = R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.NO_THING_LIKE_THIS"
                            }
                        }
                    ]
                },
                {
                    Index: 1
                    Name: "F2"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.NO_THING_LIKE_THIS"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(freshConfigStr, /* ignoreUnknownRules */ true), TValidationException);
    }
}
