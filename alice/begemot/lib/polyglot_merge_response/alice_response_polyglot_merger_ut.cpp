#include <alice/begemot/lib/polyglot_merge_response/alice_response_polyglot_merger.h>
#include <alice/library/proto/proto.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/maybe.h>

using namespace NAlice;

namespace {

using TAliceParsedFramesConfig = TAliceResponsePolyglotMergerConfig::TAliceParsedFramesConfig;
using EMergeMode = TAliceParsedFramesConfig::EMergeMode;

TAliceResponsePolyglotMergerConfig CreateMergerConfig() {
    return ParseProtoText<TAliceResponsePolyglotMergerConfig>(R"(
AliceParsedFrames {
    Name: "frame_native"
    MergeMode: Default
}
AliceParsedFrames {
    Name: "frame_both"
    MergeMode: Default
}
AliceParsedFrames {
    Name: "frame_translated"
    MergeMode: Default
}
)");
}

TFrameAggregatorConfig CreateFrameAggregatorConfig(const TVector<TString>& frameNames = {}) {
    auto result = TFrameAggregatorConfig();
    for (const auto& frameName : frameNames) {
        result.AddFrames()->SetName(frameName);
    }
    return result;
}

::NBg::NProto::TAliceResponseResult CreateNativeAliceResponse() {
    return ParseProtoText<::NBg::NProto::TAliceResponseResult>(R"(
AliceParsedFrames {
    Frames {
        Name: "frame_native"
    }
    Frames {
        Name: "frame_both"
    }
    Sources: ["native", "native"]
    Confidences: [1, 1]
}
AliceNormalizer {
    NormalizedRequest: "native request"
}
)");
}

::NBg::NProto::TAliceResponseResult CreateTranslatedAliceResponse() {
    return ParseProtoText<::NBg::NProto::TAliceResponseResult>(R"(
AliceParsedFrames {
    Frames {
        Name: "frame_both"
    }
    Frames {
        Name: "frame_translated"
    }
    Sources: ["translated", "translated"]
    Confidences: [1, 1]
}
AliceNormalizer {
    NormalizedRequest: "translated request"
}
)");
}

::NBg::NProto::TAlicePolyglotMergeResponseResult CreatePolyglotMergeResponse() {
    return ParseProtoText<::NBg::NProto::TAlicePolyglotMergeResponseResult>(R"(
AliceResponse {
    AliceParsedFrames {
        Frames {
            Name: "frame_native"
        }
        Frames {
            Name: "frame_both"
        }
        Frames {
            Name: "frame_translated"
        }
        Sources: ["native", "native", "translated"]
        Confidences: [1, 1, 1]
    }
    AliceNormalizer {
        NormalizedRequest: "native request"
    }
}
TranslatedResponse {
    AliceNormalizer: {
        NormalizedRequest: "translated request"
    }
}
)");
}

void RemoveFrame(::NBg::NProto::TAlicePolyglotMergeResponseResult& polyglotMergeResponse, const size_t index) {
    auto& aliceParsedFrames = *polyglotMergeResponse.MutableAliceResponse()->MutableAliceParsedFrames();

    aliceParsedFrames.MutableFrames()->erase(aliceParsedFrames.GetFrames().begin() + index);
    aliceParsedFrames.MutableSources()->erase(aliceParsedFrames.GetSources().begin() + index);
    aliceParsedFrames.MutableConfidences()->erase(aliceParsedFrames.GetConfidences().begin() + index);
}

} // namespace

class TAliceResponsePolyglotMergerFixture : public NUnitTest::TBaseFixture {
public:
    TAliceResponsePolyglotMergerFixture() {
        MergerConfig() = CreateMergerConfig();
        FrameAggregatorConfig() = CreateFrameAggregatorConfig();
        NativeAliceResponse() = CreateNativeAliceResponse();
        TranslatedAliceResponse() = CreateTranslatedAliceResponse();
    }

    TAliceResponsePolyglotMergerConfig& MergerConfig() {
        return MergerConfig_;
    }

    TFrameAggregatorConfig& FrameAggregatorConfig() {
        return FrameAggregatorConfig_;
    }

    ::NBg::NProto::TAliceResponseResult& NativeAliceResponse() {
        return NativeAliceResponse_;
    }

    TMaybe<::NBg::NProto::TAliceResponseResult>& TranslatedAliceResponse() {
        return TranslatedAliceResponse_;
    }

    TMaybe<EMergeMode>& ForceFrameMergeMode() {
        return ForceFrameMergeMode_;
    }

    bool& IsLogEnabled() {
        return IsLogEnabled_;
    }

    ::NBg::NProto::TAlicePolyglotMergeResponseResult RunMerger() const {
        ::NBg::NProto::TAlicePolyglotMergeResponseResult result;
        TAliceResponsePolyglotMerger(MergerConfig_, FrameAggregatorConfig_, ForceFrameMergeMode_, IsLogEnabled_)
            .MergeAliceResponses(NativeAliceResponse_, TranslatedAliceResponse_.Get(), result);
        return result;
    }
private:
    TAliceResponsePolyglotMergerConfig MergerConfig_;
    TFrameAggregatorConfig FrameAggregatorConfig_;
    TMaybe<EMergeMode> ForceFrameMergeMode_;
    bool IsLogEnabled_ = false;

    ::NBg::NProto::TAliceResponseResult NativeAliceResponse_;
    TMaybe<::NBg::NProto::TAliceResponseResult> TranslatedAliceResponse_;
};

Y_UNIT_TEST_SUITE_F(AliceResponsePolyglotMerger, TAliceResponsePolyglotMergerFixture) {
    Y_UNIT_TEST(TestSmoke) {
        UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), CreatePolyglotMergeResponse());
    }

    Y_UNIT_TEST(TestWithoutMergerConfig) {
        MergerConfig().Clear();
        UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), CreatePolyglotMergeResponse());
    }

    Y_UNIT_TEST(TestWithoutTranslated) {
        TranslatedAliceResponse().Clear();

        auto expected = CreatePolyglotMergeResponse();
        RemoveFrame(expected, 2);
        expected.ClearTranslatedResponse();

        UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), expected);
    }

    Y_UNIT_TEST(TestMergerConfig) {
        {
            MergerConfig() = CreateMergerConfig();
            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), CreatePolyglotMergeResponse());
        }

        {
            MergerConfig() = CreateMergerConfig();
            MergerConfig().MutableAliceParsedFrames()->Mutable(1)->SetMergeMode(TAliceParsedFramesConfig::NativeOnly);
            MergerConfig().MutableAliceParsedFrames()->Mutable(2)->SetMergeMode(TAliceParsedFramesConfig::NativeOnly);

            auto expected = CreatePolyglotMergeResponse();
            RemoveFrame(expected, 2);

            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), expected);
        }

        {
            MergerConfig() = CreateMergerConfig();
            MergerConfig().MutableAliceParsedFrames()->Mutable(0)->SetMergeMode(TAliceParsedFramesConfig::TranslatedOnly);
            MergerConfig().MutableAliceParsedFrames()->Mutable(1)->SetMergeMode(TAliceParsedFramesConfig::TranslatedOnly);

            auto expected = CreatePolyglotMergeResponse();
            *expected.MutableAliceResponse()->MutableAliceParsedFrames()->MutableSources()->Mutable(1) = "translated";
            RemoveFrame(expected, 0);

            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), expected);
        }

        {
            MergerConfig() = CreateMergerConfig();
            MergerConfig().MutableAliceParsedFrames()->Mutable(1)->SetMergeMode(TAliceParsedFramesConfig::NativeOrTranslated);

            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), CreatePolyglotMergeResponse());
        }

        {
            MergerConfig() = CreateMergerConfig();
            MergerConfig().MutableAliceParsedFrames()->Mutable(1)->SetMergeMode(TAliceParsedFramesConfig::TranslatedOrNative);

            auto expected = CreatePolyglotMergeResponse();
            *expected.MutableAliceResponse()->MutableAliceParsedFrames()->MutableSources()->Mutable(1) = "translated";

            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), expected);
        }

        {
            MergerConfig() = CreateMergerConfig();
            MergerConfig().MutableAliceParsedFrames()->Mutable(0)->SetMergeMode(TAliceParsedFramesConfig::NativeOnlyIfExists);
            MergerConfig().MutableAliceParsedFrames()->Mutable(1)->SetMergeMode(TAliceParsedFramesConfig::NativeOnlyIfExists);
            MergerConfig().MutableAliceParsedFrames()->Mutable(2)->SetMergeMode(TAliceParsedFramesConfig::NativeOnlyIfExists);

            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), CreatePolyglotMergeResponse());
        }
    }

    Y_UNIT_TEST(TestExistsInNativeButNotPresent) {
        {
            FrameAggregatorConfig() = CreateFrameAggregatorConfig({"frame_native", "frame_both"});
            MergerConfig().MutableAliceParsedFrames()->Mutable(1)->SetMergeMode(TAliceParsedFramesConfig::Default);
            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), CreatePolyglotMergeResponse());
        }

        {
            FrameAggregatorConfig() = CreateFrameAggregatorConfig({"frame_native", "frame_both", "frame_translated"});
            MergerConfig().MutableAliceParsedFrames()->Mutable(1)->SetMergeMode(TAliceParsedFramesConfig::Default);

            auto expected = CreatePolyglotMergeResponse();
            RemoveFrame(expected, 2);

            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), expected);
        }

        {
            FrameAggregatorConfig() = CreateFrameAggregatorConfig({"frame_native", "frame_both", "frame_translated"});
            MergerConfig().MutableAliceParsedFrames()->Mutable(2)->SetMergeMode(TAliceParsedFramesConfig::NativeOrTranslated);
            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), CreatePolyglotMergeResponse());
        }
    }

    Y_UNIT_TEST(TestForceFrameMergeMode) {
        {
            ForceFrameMergeMode() = Nothing();
            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), CreatePolyglotMergeResponse());
        }

        {
            ForceFrameMergeMode() = TAliceParsedFramesConfig::NativeOnly;

            auto expected = CreatePolyglotMergeResponse();
            RemoveFrame(expected, 2);

            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), expected);
        }

        {
            ForceFrameMergeMode() = TAliceParsedFramesConfig::TranslatedOnly;

            auto expected = CreatePolyglotMergeResponse();
            *expected.MutableAliceResponse()->MutableAliceParsedFrames()->MutableSources()->Mutable(1) = "translated";
            RemoveFrame(expected, 0);

            UNIT_ASSERT_MESSAGES_EQUAL(RunMerger(), expected);
        }
    }

    Y_UNIT_TEST(TestLog) {
        {
            IsLogEnabled() = false;
            const auto result = RunMerger();
            UNIT_ASSERT(result.GetLog().empty());
        }
        {
            IsLogEnabled() = true;
            const auto result = RunMerger();

            UNIT_ASSERT(!result.GetLog().empty());
            UNIT_ASSERT(AnyOf(result.GetLog(), [](const TString& log) { return log.Contains("frame_native"); }));
            UNIT_ASSERT(AnyOf(result.GetLog(), [](const TString& log) { return log.Contains("frame_both"); }));
            UNIT_ASSERT(AnyOf(result.GetLog(), [](const TString& log) { return log.Contains("frame_translated"); }));
        }
    }
}
