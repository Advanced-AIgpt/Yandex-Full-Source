#include <alice/begemot/lib/trivial_tagger/config.h>
#include <alice/begemot/lib/trivial_tagger/tagger.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(TrivialTagger) {

    const TStringBuf StaticConfigStr = R"(
        {
          "frames": [
            {
              "name": "alice.demo_intent_1",
              "slots": [
                {
                  "name": "demo_slot",
                  "type": "custom.demo_entity",
                  "value": "raw_string_value"
                }
              ]
            }
          ]
        }
    )";

    const TStringBuf FreshConfigStr = R"(
        {
          "frames": [
            {
              "name": "alice.demo_intent_1",
              "slots": [
                {
                  "name": "demo_slot",
                  "type": "custom.demo_entity",
                  "value_json": {
                    "k1": 1,
                    "k2": {
                      "k3": "v3"
                    }
                  }
                },
                {
                  "name": "demo_slot_2",
                  "type": "custom.demo_entity_2",
                  "value": "raw_string_value_2"
                }
              ]
            },
            {
              "name": "alice.demo_intent_2",
              "experiments": ["bg_enable_demo_intent_2"]
            }
          ]
        }
    )";

    const TStringBuf ConfigPatchStr = R"(
        {
          "frames": [
            {
              "name": "alice.demo_intent_1",
              "slots": [
                {
                  "name": "demo_slot_3",
                  "type": "custom.demo_entity_3",
                  "value": "raw_string_value_3"
                }
              ]
            },
            {
              "name": "alice.demo_intent_3"
            }
          ]
        }
    )";

    TTrivialTaggerConfig MakeTestConfig(bool addPatch, TStringBuf freshOptionsStr) {
        const TTrivialTaggerConfig staticConfig = ReadTrivialTaggerConfigFromJsonString(StaticConfigStr);
        const TTrivialTaggerConfig freshConfig = ReadTrivialTaggerConfigFromJsonString(FreshConfigStr);
        TVector<TTrivialTaggerConfig> configPatches;
        if (addPatch) {
            configPatches.push_back(ReadTrivialTaggerConfigFromJsonString(ConfigPatchStr));
        }
        const TFreshOptions freshOptions = ParseProtoText<TFreshOptions>(freshOptionsStr);
        return MergeTrivialTaggerConfigs(&staticConfig, &freshConfig, configPatches, freshOptions);
    }

    TVector<TSemanticFrame> BuildTestFrames(bool addPatch, TStringBuf freshOptionsStr,
        const THashSet<TString>& experiments)
    {
        return BuildTrivialTaggerFrames(MakeTestConfig(addPatch, freshOptionsStr), experiments);
    }

    Y_UNIT_TEST(Config) {
        const TTrivialTaggerConfig config = MakeTestConfig(false, "");
        UNIT_ASSERT_EQUAL(config.FramesSize(), 1);
        UNIT_ASSERT_EQUAL(config.GetFrames(0).GetName(), "alice.demo_intent_1");
        UNIT_ASSERT_EQUAL(config.GetFrames(0).SlotsSize(), 1);
        UNIT_ASSERT_EQUAL(config.GetFrames(0).GetSlots(0).GetName(), "demo_slot");
        UNIT_ASSERT_EQUAL(config.GetFrames(0).GetSlots(0).GetType(), "custom.demo_entity");
        UNIT_ASSERT_EQUAL(config.GetFrames(0).GetSlots(0).GetValue(), "raw_string_value");
    }

    Y_UNIT_TEST(Static) {
        const TVector<TSemanticFrame> frames = BuildTrivialTaggerFrames(MakeTestConfig(false, ""), {});
        UNIT_ASSERT_EQUAL(frames.size(), 1);
        UNIT_ASSERT_EQUAL(frames[0].GetName(), "alice.demo_intent_1");
        UNIT_ASSERT_EQUAL(frames[0].SlotsSize(), 1);
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(0).GetName(), "demo_slot");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(0).GetType(), "custom.demo_entity");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(0).GetValue(), "raw_string_value");
    }

    Y_UNIT_TEST(ForceEntireFresh) {
        const TVector<TSemanticFrame> frames = BuildTestFrames(false, "ForceEntireFresh: True", {});
        UNIT_ASSERT_EQUAL(frames.size(), 1);
        UNIT_ASSERT_EQUAL(frames[0].GetName(), "alice.demo_intent_1");
        UNIT_ASSERT_EQUAL(frames[0].SlotsSize(), 2);
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(0).GetName(), "demo_slot");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(0).GetType(), "custom.demo_entity");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(0).GetValue(), R"({"k1":1,"k2":{"k3":"v3"}})");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(1).GetName(), "demo_slot_2");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(1).GetType(), "custom.demo_entity_2");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(1).GetValue(), "raw_string_value_2");
    }

    Y_UNIT_TEST(ForceEntireFreshWithExperiment) {
        const TVector<TSemanticFrame> frames = BuildTestFrames(false, "ForceEntireFresh: True", {"bg_enable_demo_intent_2"});
        UNIT_ASSERT_EQUAL(frames.size(), 2);
        UNIT_ASSERT_EQUAL(frames[0].GetName(), "alice.demo_intent_1");
        UNIT_ASSERT_EQUAL(frames[1].GetName(), "alice.demo_intent_2");
    }

    Y_UNIT_TEST(ForceFreshForPrefixStrict) {
        const TVector<TSemanticFrame> frames = BuildTestFrames(false, R"(ForceForPrefixes: ["alice.demo_intent_1"])", {"bg_enable_demo_intent_2"});
        UNIT_ASSERT_EQUAL(frames.size(), 1);
        UNIT_ASSERT_EQUAL(frames[0].GetName(), "alice.demo_intent_1");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(0).GetValue(), R"({"k1":1,"k2":{"k3":"v3"}})");
    }

    Y_UNIT_TEST(ForceFreshForPrefixWide) {
        const TVector<TSemanticFrame> frames = BuildTestFrames(false, R"(ForceForPrefixes: ["alice.demo_intent"])", {"bg_enable_demo_intent_2"});
        UNIT_ASSERT_EQUAL(frames.size(), 2);
        UNIT_ASSERT_EQUAL(frames[0].GetName(), "alice.demo_intent_1");
        UNIT_ASSERT_EQUAL(frames[1].GetName(), "alice.demo_intent_2");
    }

    Y_UNIT_TEST(ConfigPatch) {
        const TVector<TSemanticFrame> frames = BuildTestFrames(true, "", {});
        UNIT_ASSERT_EQUAL(frames.size(), 2);
        UNIT_ASSERT_EQUAL(frames[0].GetName(), "alice.demo_intent_1");
        UNIT_ASSERT_EQUAL(frames[1].GetName(), "alice.demo_intent_3");
        UNIT_ASSERT_EQUAL(frames[0].GetSlots(0).GetValue(), "raw_string_value_3");
    }
}
