#include <alice/begemot/lib/feature_aggregator/aggregator.h>
#include <alice/begemot/lib/feature_aggregator/config.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NFeatureAggregator;

NBg::NProto::TAliceEntitiesCollectorResult PrepareEntitiesCollectorResponse() {
    return NAlice::ParseProtoText<NBg::NProto::TAliceEntitiesCollectorResult>(R"(
        Entities: [
            {Begin: 0 End: 1 Type: "custom.video_selection_action" Value: "play"},
            {Begin: 0 End: 1 Type: "custom.video_action" Value: "play"},
            {Begin: 0 End: 1 Type: "custom.launch_command" Value: "launch_command"},
            {Begin: 0 End: 1 Type: "custom.action_request" Value: "autoplay"},
            {Begin: 1 End: 2 Type: "custom.news_topic" Value: "music"},
            {Begin: 1 End: 2 Type: "custom.player_type" Value: "music"},
            {Begin: 3 End: 4 Type: "custom.popular_goods" Value: "popular"},
            {Begin: 0 End: 1 Type: "device.iot.bow_action" Value: "включать"},
            {Begin: 2 End: 3 Type: "device.iot.bow_action" Value: "на"},
            {Begin: 2 End: 3 Type: "device.iot.preposition" Value: "на"},
            {Begin: 3 End: 4 Type: "device.iot.room" Value: "d4c620e7-4ac6-48ca-9ba4-4bfad3c61ef8"},
            {Begin: 2 End: 4 Type: "sys.album" Value: "\"album\""},
            {Begin: 1 End: 2 Type: "sys.films_100_750" Value: "\"movie\""},
            {Begin: 3 End: 4 Type: "sys.films_100_750" Value: "\"movie\""},
            {Begin: 3 End: 4 Type: "sys.films_50_filtered" Value: "\"movie\""},
            {Begin: 1 End: 2 Type: "sys.soft" Value: "\"apple music\""},
            {Begin: 2 End: 4 Type: "sys.track" Value: "\"track\""}
        ]
    )");
}

NBg::NProto::TAliceGcDssmClassifierResult PrepareGcDssmClassifierResponse() {
    return NAlice::ParseProtoText<NBg::NProto::TAliceGcDssmClassifierResult>(R"(Score: 0.9)");
}

NBg::NProto::TAliceParsedFramesResult PrepareParsedFramesResponse() {
    return NAlice::ParseProtoText<NBg::NProto::TAliceParsedFramesResult>(R"(
        Frames: [
            {
                Name: "alice.iot.turn_on_device_type"
                Slots: [
                    {
                        Name: "device_type"
                        Type: "user.iot.device_type"
                        Value: "devices.types.media_device.tv"
                        AcceptedTypes: [
                            "user.iot.device_type",
                            "device.iot.device_type"
                        ]
                    }
                ]
            },
            {
                Name: "alice.screen_on"
            },
            {
                Name: "personal_assistant.scenarios.tv_stream"
                Slots: [
                    {
                        Name: "tv_action"
                        Type: "string"
                        Value: "включи"
                        AcceptedTypes: [
                            "string"
                        ]
                    }
                ]
            }
        ]
        Confidences: [
            1,
            1,
            0.4568
        ],
        Sources: [
            "Granet",
            "Granet",
            "AliceTagger"
        ]
    )");
}

NBg::NProto::TAliceWizDetectionResult PrepareWizDetectionResponse() {
    return NAlice::ParseProtoText<NBg::NProto::TAliceWizDetectionResult>(R"(
        ModelResults: [
            {
                Name: "shinyserp_politota"
                Passed: false
                Probability: 0.0385931842
                Updated: "1604307851"
            },
            {
                Name: "shinyserp_unethical"
                Passed: true
                Probability: 0.999
                Updated: "1604307851"
            },
            {
                Name: "shinyserp_porno"
                Passed: false
                Probability: 0.01036472712
                Updated: "1604307851"
            }
        ]
    )");
}

NBg::NProto::TAliceMultiIntentClassifierResult PrepareMultiIntentClassifierResponse() {
    return NAlice::ParseProtoText<NBg::NProto::TAliceMultiIntentClassifierResult>(R"(
        ClassifiersResult: {
            key: "generic_scenarios"
            value: {
                Probabilities: {
                    key: "music"
                    value : 0.5
                }
                Probabilities: {
                    key: "video"
                    value: 0.3
                }
                Probabilities: {
                    key: "weather"
                    value: 0.2
                }
            }
        }
        ClassifiersResult: {
            key: "TOLOKA_WORD_LSTM"
            value: {
                Probabilities: {
                    key: "personal_assistant.scenarios.open_site_or_app"
                    value : 0.2
                }
                Probabilities: {
                    key: "personal_assistant.scenarios.music_sing_song"
                    value: 0.1
                }
                Probabilities: {
                    key: "personal_assistant.scenarios.quasar.open_current_video"
                    value: 0.6
                }
            }
        }
    )");
}

NBg::NProto::TPornQueryResult PreparePornQueryResponse() {
    return NAlice::ParseProtoText<NBg::NProto::TPornQueryResult>(R"(
        WebIpqThreshold: 0,
        SourceLabel: "video_old: ipq_2019_08_01T13_21_43_20th.gzt.bin image_old: ipq_2019_08_01T13_21_43_20th.gzt.bin web: ipq_2019_12_01T17_24_09.gzt.bin"
        ImageConfidence: 0.9984999895
        ImageIsPornoQuery: 1
        VideoIsPornoQuery: 1
        IsPornoQuery: 1
        VideoConfidence: 0.9984999895
        Confidence: 0.4483750463
    )");
}

TVector<float> AggregateVector(const TFeatureAggregator& aggregator,
                               const TFeatureAggregatorSources& sources,
                               const THashSet<TString>& experiments,
                               const NAlice::TFreshOptions options = NAlice::TFreshOptions()) {

    const google::protobuf::RepeatedField<float> protoResult = aggregator.Aggregate(sources, experiments, options).Features.GetFeatures();
    return {protoResult.begin(), protoResult.end()};
}


Y_UNIT_TEST_SUITE(RulesIsolated) {

    Y_UNIT_TEST(EmptyFeatures) {
        TFeatureAggregatorSources sources;
        const THashSet<TString> experiments;
        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: []
            )")
        );

        // 0-indexed feature is reserved one
        const TVector<float> expected = {0.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, experiments));
    }

    Y_UNIT_TEST(EntitiesRule) {
        const auto response = PrepareEntitiesCollectorResponse();
        TFeatureAggregatorSources sources;
        sources.AliceEntities = &response;
        const THashSet<TString> experiments;
        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                Entity: {
                                    EntityType: "sys.album"
                                }
                            }
                        ]
                    },
                    {
                        Index: 2
                        Name: "F2"
                        Rules: [
                            {
                                Entity: {
                                    EntityType: "sys.date"
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        const TVector<float> expected = {0.0F, 1.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, experiments));
    }

    Y_UNIT_TEST(GranetFrameRule) {
        const auto response = PrepareParsedFramesResponse();
        TFeatureAggregatorSources sources;
        sources.AliceParsedFrames = &response;
        const THashSet<TString> experiments;
        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                GranetFrame: {
                                    FrameName: "alice.screen_on"
                                }
                            }
                        ]
                    },
                    {
                        Index: 2
                        Name: "F2"
                        Rules: [
                            {
                                GranetFrame: {
                                    FrameName: "personal_assistant.scenarios.tv_stream"
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        const TVector<float> expected = {0.0F, 1.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, experiments));
    }

    Y_UNIT_TEST(GcDssmRule) {
        const auto response = PrepareGcDssmClassifierResponse();
        TFeatureAggregatorSources sources;
        sources.AliceGcDssmClassifier = &response;
        const THashSet<TString> experiments;
        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                BinaryClassifier: {
                                    Classifier: ALICE_GC_DSSM
                                    UseRawValue: true
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        const TVector<float> expected = {0.0F, 0.9F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, experiments));
    }

    Y_UNIT_TEST(WizDetectionRule) {
        const auto response = PrepareWizDetectionResponse();
        TFeatureAggregatorSources sources;
        sources.AliceWizDetection = &response;
        const THashSet<TString> experiments;
        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                WizDetection: {
                                    FeatureName: "shinyserp_unethical"
                                }
                            }
                        ]
                    },
                    {
                        Index: 2
                        Name: "F2"
                        Rules: [
                            {
                                WizDetection: {
                                    FeatureName: "shinyserp_politota"
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        const TVector<float> expected = {0.0F, 1.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, experiments));
    }

    Y_UNIT_TEST(MultiIntentClassifierRule) {
        const auto response = PrepareMultiIntentClassifierResponse();
        TFeatureAggregatorSources sources;
        sources.AliceMultiIntentClassifier = &response;
        const THashSet<TString> experiments;
        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "generic_scenarios"
                                    IntentName: "video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    },
                    {
                        Index: 2
                        Name: "F2"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "generic_scenarios"
                                    IntentName: "not_existing"
                                    UseRawValue: true
                                }
                            }
                        ]
                    },
                    {
                        Index: 3
                        Name: "F3"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "not_existing"
                                    IntentName: "not_existing"
                                    UseRawValue: true
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        const TVector<float> expected = {0.0F, 0.3F, 0.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, experiments));
    }

    Y_UNIT_TEST(PornQueryRule) {
        const auto response = PreparePornQueryResponse();
        TFeatureAggregatorSources sources;
        sources.PornQuery = &response;
        const THashSet<TString> experiments;
        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                PornQuery: {}
                            }
                        ]
                    }
                ]
            )")
        );

        const TVector<float> expected = {0.0F, 1.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, experiments));
    }
}

Y_UNIT_TEST_SUITE(RulesPriority) {

    Y_UNIT_TEST(ExperimentsPriority) {
        const auto clfResponse = PrepareMultiIntentClassifierResponse();
        const auto wizDetection = PrepareWizDetectionResponse();

        TFeatureAggregatorSources sources;
        sources.AliceMultiIntentClassifier = &clfResponse;
        sources.AliceWizDetection = &wizDetection;

        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                Experiments: ["E2", "E1"]
                                WizDetection: {
                                    FeatureName: "shinyserp_unethical"
                                }
                            },
                            {
                                Experiments: ["E2"]
                                WizDetection: {
                                    FeatureName: "shinyserp_politota"
                                }
                            },
                            {
                                Experiments: ["E1"]
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.music_sing_song"
                                    UseRawValue: true
                                }
                            },
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        TVector<float> expected = {0.0F, 1.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"E1", "E2", "E3"}));

        expected = {0.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"E2"}));

        expected = {0.0F, 0.1F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"E1"}));

        expected = {0.0F, 0.6F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {}));
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"E3", "E4"}));
    }

    Y_UNIT_TEST(ExperimentsPriorityNoSource) {
        const auto wizDetection = PrepareWizDetectionResponse();

        TFeatureAggregatorSources sources;
        sources.AliceWizDetection = &wizDetection;

        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                Experiments: ["E2", "E1"]
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.music_sing_song"
                                    UseRawValue: true
                                }
                            },
                            {
                                Experiments: ["E2"]
                                WizDetection: {
                                    FeatureName: "shinyserp_unethical"
                                }
                            },
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        // triggers wiz detection rule
        TVector<float> expected = {0.0F, 1.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"E1", "E2"}));
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"E2"}));

        // triggers last rule (0.6F), but no source, default val is expected
        expected = {0.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"E1"}));
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {}));
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"E3", "E4"}));
    }
}

Y_UNIT_TEST_SUITE(Misc) {

    Y_UNIT_TEST(IsDisabled) {
        const auto clfResponse = PrepareMultiIntentClassifierResponse();

        TFeatureAggregatorSources sources;
        sources.AliceMultiIntentClassifier = &clfResponse;

        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    },
                    {
                        Index: 2
                        Name: "F2"
                        IsDisabled: true
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    },
                    {
                        Index: 3
                        Name: "F3"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        TVector<float> expected = {0.0F, 0.6F, 0.0F, 0.6F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {}));
    }

    Y_UNIT_TEST(SparseIndexes) {
        const auto clfResponse = PrepareMultiIntentClassifierResponse();

        TFeatureAggregatorSources sources;
        sources.AliceMultiIntentClassifier = &clfResponse;

        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    },
                    {
                        Index: 100
                        Name: "F100"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    },
                    {
                        Index: 200
                        Name: "F200"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                            }
                        ]
                    }
                ]
            )")
        );

        TVector<float> expected(201, 0.0F);
        expected[1] = 0.6F;
        expected[100] = 0.6F;
        expected[200] = 0.6F;

        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {}));
    }

    Y_UNIT_TEST(Thresholds) {
        const auto clfResponse = PrepareMultiIntentClassifierResponse();

        TFeatureAggregatorSources sources;
        sources.AliceMultiIntentClassifier = &clfResponse;

        const auto aggregator = TFeatureAggregator(
            ReadConfigFromProtoTxtString(R"(
                Features: [
                    {
                        Index: 1
                        Name: "F1"
                        Rules: [
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    UseRawValue: true
                                }
                                Experiments: ["NO_THRESH"]
                            },
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    IntervalMapping: {
                                        ValueBefore: 0.0
                                        Points: [
                                            {
                                                Threshold: 0.7
                                                ValueAfter: 1.0
                                            }
                                        ]
                                    }
                                }
                                Experiments: ["LT_THRESH"]
                            },
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    IntervalMapping: {
                                        ValueBefore: 0.0
                                        Points: [
                                            {
                                                Threshold: 0.6
                                                ValueAfter: 1.0
                                            }
                                        ]
                                    }
                                }
                                Experiments: ["EQ_THRESH"]
                            },
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
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
                                Experiments: ["GT_THRESH"]
                            },
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    IntervalMapping: {
                                        ValueBefore: 0.0
                                        Points: [
                                            {
                                                Threshold: 0.5
                                                ValueAfter: 1.0
                                            },
                                            {
                                                Threshold: 0.55
                                                ValueAfter: 1.5
                                            },
                                            {
                                                Threshold: 0.8
                                                ValueAfter: 2.0
                                            }
                                        ]
                                    }
                                }
                                Experiments: ["MID_THRESH"]
                            },
                            {
                                MultiIntentClassifier: {
                                    Classifier: "TOLOKA_WORD_LSTM"
                                    IntentName: "personal_assistant.scenarios.quasar.open_current_video"
                                    IntervalMapping: {
                                        ValueBefore: -3.0
                                        Points: [
                                            {
                                                Threshold: 0.7
                                                ValueAfter: 1.0
                                            },
                                            {
                                                Threshold: 0.8
                                                ValueAfter: 1.5
                                            }
                                        ]
                                    }
                                }
                                Experiments: ["VALUE_BEFORE"]
                            }
                        ]
                    }
                ]
            )")
        );

        TVector<float> expected = {0.0F, 0.6F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"NO_THRESH"}));

        expected = {0.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"LT_THRESH"}));

        expected = {0.0F, 1.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"EQ_THRESH"}));
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"GT_THRESH"}));

        expected = {0.0F, 1.5F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"MID_THRESH"}));

        expected = {0.0F, -3.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, {"VALUE_BEFORE"}));
    }
}

Y_UNIT_TEST_SUITE(FreshData) {
    Y_UNIT_TEST(BadSourseTypeAllowed) {
        const auto entities = PrepareEntitiesCollectorResponse();

        TFeatureAggregatorSources sources;
        sources.AliceEntities = &entities;

        const THashSet<TString> experiments = {"EmptyRule"};

        auto staticConfig = ReadConfigFromProtoTxtString(R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                }
            ]
        )");

        auto freshConfigStr = R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Something: {
                                Somebody: "enotropy"
                            }
                            Experiments: ["EmptyRule"]
                        },
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                }
            ]
        )";

        UNIT_ASSERT_EXCEPTION(ReadConfigFromProtoTxtString(freshConfigStr), NAlice::TParseProtoTextError);

        const auto aggregator = TFeatureAggregator(staticConfig, ReadConfigFromProtoTxtString(freshConfigStr, /* ignoreUnknownRules */ true));

        const TVector<float> expected = {0.0F, 1.0F};
        UNIT_ASSERT_EQUAL(expected, AggregateVector(aggregator, sources, experiments));
    }

    Y_UNIT_TEST(FreshPrefixes) {
        const auto entities = PrepareEntitiesCollectorResponse();

        TFeatureAggregatorSources sources;
        sources.AliceEntities = &entities;

        const THashSet<TString> experiments = {};

        auto staticConfig = ReadConfigFromProtoTxtString(R"(
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
                    Name: "T1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.NO_THING_LIKE_THIS"
                            }
                        }
                    ]
                }
            ]
        )");

        auto freshConfig = ReadConfigFromProtoTxtString(R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "T1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                }
            ]
        )");

        const auto aggregator = TFeatureAggregator(staticConfig, freshConfig);

        const TVector<float> expectedStatic = {0.0F, 0.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expectedStatic, AggregateVector(aggregator, sources, experiments));

        NAlice::TFreshOptions options;
        options.AddForceForPrefixes("F");

        auto aggregateResult = aggregator.Aggregate(sources, experiments, options);
        UNIT_ASSERT_EQUAL(false, aggregateResult.UsedEntireFresh);
        const TVector<TString> expectedFreshFeatures = {"F1"};
        UNIT_ASSERT_EQUAL(aggregateResult.UsedFreshForFeatures, expectedFreshFeatures);

        const TVector<float> expectedFresh = {0.0F, 1.0F, 0.0F};
        const auto& featuresAsRepeatedField = aggregateResult.Features.GetFeatures();
        const TVector<float> features = {featuresAsRepeatedField.begin(), featuresAsRepeatedField.end()};
        UNIT_ASSERT_EQUAL(expectedFresh, features);
    }

    Y_UNIT_TEST(ForceEntireFresh) {
         const auto entities = PrepareEntitiesCollectorResponse();

        TFeatureAggregatorSources sources;
        sources.AliceEntities = &entities;

        const THashSet<TString> experiments = {};

        auto staticConfig = ReadConfigFromProtoTxtString(R"(
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
                    Name: "T1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.NO_THING_LIKE_THIS"
                            }
                        }
                    ]
                }
            ]
        )");

        auto freshConfig = ReadConfigFromProtoTxtString(R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "T1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                }
            ]
        )");

        const auto aggregator = TFeatureAggregator(staticConfig, freshConfig);

        const TVector<float> expectedStatic = {0.0F, 0.0F, 0.0F};
        UNIT_ASSERT_EQUAL(expectedStatic, AggregateVector(aggregator, sources, experiments));

        NAlice::TFreshOptions options;
        options.SetForceEntireFresh(true);

        const TVector<float> expectedFresh = {0.0F, 1.0F, 1.0F};
        UNIT_ASSERT_EQUAL(expectedFresh, AggregateVector(aggregator, sources, experiments, options));
    }

    Y_UNIT_TEST(StaticBiggerThanFresh) {
         const auto entities = PrepareEntitiesCollectorResponse();

        TFeatureAggregatorSources sources;
        sources.AliceEntities = &entities;

        const THashSet<TString> experiments = {};

        auto staticConfig = ReadConfigFromProtoTxtString(R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "F2"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                },
                {
                    Index: 3
                    Name: "F3"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                }
            ]
        )");

        auto freshConfig = ReadConfigFromProtoTxtString(R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "F2"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "sys.album"
                            }
                        }
                    ]
                }
            ]
        )");

        UNIT_ASSERT_EXCEPTION(TFeatureAggregator(staticConfig, freshConfig), TValidationException);
    }
}
