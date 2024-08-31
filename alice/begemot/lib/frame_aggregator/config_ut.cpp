#include <alice/begemot/lib/frame_aggregator/config.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(Validate) {

    Y_UNIT_TEST(OK) {
        const TStringBuf config = R"(
          Frames: [
            {
              Name: "alice.market.how_much"
              Experiments: ["bg_market_how_much_on_binary_classifier"]
              Rules: [
                {
                  Classifier: {
                    Source: "AliceBinaryIntentClassifier"
                    Threshold: 0.6
                    Confidence: 1
                  }
                  Tagger: {
                    Source: "Granet"
                  }
                }
              ]
            },
            {
              Name: "alice.market.how_much"
              Experiments: ["bg_market_how_much_on_multi_classifier"]
              Rules: [
                {
                  Classifier: {
                    Source: "AliceScenariosWordLstm"
                    Intent: "personal_assistant.scenarios.how_much"
                    Threshold: 0.5
                    Confidence: 1
                  }
                  Tagger: {
                    Source: "Granet"
                  }
                }
              ]
            },
            {
              Name: "alice.test"
              Rules: [
                {
                  Classifier: {
                    Source: "Always"
                  }
                  Tagger: {
                    Source: "Always"
                  }
                }
              ]
            },
            {
              Name: "alice.test.classifier_only"
              Rules: [
                {
                  Classifier: {
                    Source: "AliceMultiIntentClassifier"
                    Model: "test_model"
                  }
                }
              ]
            },
            {
              Name: "alice.test.wiz_classifier"
              Rules: [
                {
                  Classifier: {
                    Source: "AliceWizDetection"
                    Model: "test_model"
                  }
                }
              ]
            }
          ]
        )";

        UNIT_ASSERT_NO_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)));
    }

    Y_UNIT_TEST(EmptyFrameName) {
        const TStringBuf config = R"(
          Frames: [
            {
            }
          ]
        )";
        UNIT_ASSERT_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)), TFrameConfigValidationException);
    }

    Y_UNIT_TEST(EmptyClassifier) {
        const TStringBuf config = R"(
          Frames: [
            {
              Name: "alice.test"
              Rules: [
                {
                  Tagger: {
                    Source: "AliceTagger"
                  }
                }
              ]
            }
          ]
        )";
        UNIT_ASSERT_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)), TFrameConfigValidationException);
    }

    Y_UNIT_TEST(InvalidClassifierSource) {
        const TStringBuf config = R"(
          Frames: [
            {
              Name: "alice.my_intent"
              Rules: [
                {
                  Classifier: {
                    Source: "InvalidSource"
                    Threshold: 0.5
                  }
                  Tagger: {
                    Source: "Granet"
                  }
                }
              ]
            }
          ]
        )";
        UNIT_ASSERT_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)), TFrameConfigValidationException);
    }

    Y_UNIT_TEST(InvalidTaggerSource) {
        const TStringBuf config = R"(
          Frames: [
            {
              Name: "alice.my_intent"
              Rules: [
                {
                  Classifier: {
                    Source: "Granet"
                    Threshold: 0.5
                  }
                  Tagger: {
                    Source: "InvalidSource"
                  }
                }
              ]
            }
          ]
        )";
        UNIT_ASSERT_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)), TFrameConfigValidationException);
    }

    Y_UNIT_TEST(MultipleRules) {
        const TStringBuf config = R"(
          Frames: [
            {
              Name: "alice.intent_with_cascade_of_classifiers"
              Rules: [
                {
                  Classifier: {
                    Source: "Granet"
                    Intent: "personal_assistant.scenarios.how_much"
                  }
                  Tagger: {
                    Source: "Granet"
                    Intent: "personal_assistant.scenarios.how_much"
                  }
                },
                {
                  Classifier: {
                    Source: "AliceBinaryIntentClassifier"
                    Intent: "personal_assistant.scenarios.how_much"
                  }
                  Tagger: {
                    Source: "AliceTagger"
                    Intent: "personal_assistant.scenarios.how_much"
                  }
                }
              ]
            }
          ]
        )";
        UNIT_ASSERT_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)), TFrameConfigValidationException);
    }

    Y_UNIT_TEST(EmptyModelAliceMultiIntentClassifier) {
        const TStringBuf config = R"(
          Frames: [
            {
              Name: "alice.test"
              Rules: [
                {
                  Classifier: {
                    Source: "AliceMultiIntentClassifier"
                    Intent: "test_intent"
                  }
                }
              ]
            }
          ]
        )";
        UNIT_ASSERT_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)), TFrameConfigValidationException);
    }

    Y_UNIT_TEST(EmptyModelAliceWizDetection) {
        const TStringBuf config = R"(
          Frames: [
            {
              Name: "alice.test"
              Rules: [
                {
                  Classifier: {
                    Source: "AliceWizDetection"
                  }
                }
              ]
            }
          ]
        )";
        UNIT_ASSERT_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)), TFrameConfigValidationException);
    }

    Y_UNIT_TEST(NonEmptyIntentAliceWizDetection) {
        const TStringBuf config = R"(
          Frames: [
            {
              Name: "alice.test"
              Rules: [
                {
                  Classifier: {
                    Source: "AliceWizDetection"
                    Model: "test_model"
                    Intent: "test_intent"
                  }
                }
              ]
            }
          ]
        )";
        UNIT_ASSERT_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)), TFrameConfigValidationException);
    }
}
