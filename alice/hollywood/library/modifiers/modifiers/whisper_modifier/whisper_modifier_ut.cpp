#include "whisper_modifier.h"

#include <alice/hollywood/library/modifiers/testing/mock_modifier_context.h>

#include <alice/library/proto/proto.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/megamind/protos/analytics/modifiers/whisper/whisper.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace testing;
using namespace NAlice;
using namespace NAlice::NHollywood;
using namespace NAlice::NHollywood::NModifiers;
using namespace NAlice::NHollywood::NModifiers::NImpl;
using namespace NAlice::NScenarios;

namespace {

Y_UNIT_TEST_SUITE(WhisperModifier) {
    Y_UNIT_TEST(Inapplicable) {
        TMockModifierContext ctx;
        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: false
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                }
            )");

            TResponseBodyBuilder responseBody{modifierBody};
            TModifierAnalyticsInfoBuilder analyticsInfo;
            auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
            UNIT_ASSERT(result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(result->Type(), TNonApply::EType::NotApplicable);
            UNIT_ASSERT_MESSAGES_EQUAL(responseBody.GetModifierBody(), modifierBody);
            const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
            UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: false
                IsSoundSetLevelDirectiveApplied: false
            )"));
        }
        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                }
            )");

            TResponseBodyBuilder responseBody{modifierBody};
            TModifierAnalyticsInfoBuilder analyticsInfo;
            auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
            UNIT_ASSERT(result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(result->Type(), TNonApply::EType::NotApplicable);
            UNIT_ASSERT_MESSAGES_EQUAL(responseBody.GetModifierBody(), modifierBody);
            const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
            UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: false
                IsSoundSetLevelDirectiveApplied: false
            )"));

        }
    }

    Y_UNIT_TEST(ApplySimpleCase) {
        TMockModifierContext ctx;
        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    MultiroomSessionId: "lessmeaning"
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            const auto baseRequest = ParseProtoText<NMegamind::TModifierRequest_TBaseRequest>(R"(
                Interfaces {
                    TtsPlayPlaceholder: true
                }
            )");
            EXPECT_CALL(ctx, GetBaseRequest()).WillRepeatedly(ReturnRef(baseRequest));

            const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                }
            )");

            TResponseBodyBuilder responseBody{modifierBody};
            TModifierAnalyticsInfoBuilder analyticsInfo;
            auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
            UNIT_ASSERT(!result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(),
                                     "sil<[100]> <speaker is_whisper=\"true\"> kek");
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetDirectives().size(), 2);
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[0].HasSoundSetLevelDirective());
            UNIT_ASSERT_MESSAGES_EQUAL(
                responseBody.GetModifierBody().GetLayout().GetDirectives()[0].GetSoundSetLevelDirective(),
                ParseProtoText<TSoundSetLevelDirective>(R"(
                    Name: "sound_set_level"
                    MultiroomSessionId: "lessmeaning"
                    NewLevel: 3
                )"));
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[1].HasTtsPlayPlaceholderDirective());
            const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
            UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: true
                IsSoundSetLevelDirectiveApplied: true
            )"));
        }

        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    SoundLevel: 1243
                    MultiroomSessionId: "lessmeaning"
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            const auto baseRequest = ParseProtoText<NMegamind::TModifierRequest_TBaseRequest>(R"(
                Interfaces {
                    TtsPlayPlaceholder: true
                }
            )");
            EXPECT_CALL(ctx, GetBaseRequest()).WillRepeatedly(ReturnRef(baseRequest));

            const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                }
            )");

            TResponseBodyBuilder responseBody{modifierBody};
            TModifierAnalyticsInfoBuilder analyticsInfo;
            auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
            UNIT_ASSERT(!result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(),
                                     "sil<[100]> <speaker is_whisper=\"true\"> kek");
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetDirectives().size(), 2);
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[0].HasSoundSetLevelDirective());
            UNIT_ASSERT_MESSAGES_EQUAL(
                responseBody.GetModifierBody().GetLayout().GetDirectives()[0].GetSoundSetLevelDirective(),
                ParseProtoText<TSoundSetLevelDirective>(R"(
                    Name: "sound_set_level"
                    MultiroomSessionId: "lessmeaning"
                    NewLevel: 3
                )"));
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[1].HasTtsPlayPlaceholderDirective());
            const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
            UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: true
                IsSoundSetLevelDirectiveApplied: true
            )"));
        }
    }


    Y_UNIT_TEST(ApplyWithImmutableSoundLevel) {
        TMockModifierContext ctx;
        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    SoundLevel: 2
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                }
            )");

            TResponseBodyBuilder responseBody{modifierBody};
            TModifierAnalyticsInfoBuilder analyticsInfo;
            auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
            UNIT_ASSERT(!result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(),
                                     "<speaker is_whisper=\"true\"> kek");
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives().empty());
            const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
            UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: true
                IsSoundSetLevelDirectiveApplied: false
            )"));
        }

        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    SoundLevel: 124
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                    Directives {
                        SoundSetLevelDirective {
                            Name: "sound_set_level"
                            MultiroomSessionId: "lessmeaning"
                            NewLevel: 333
                        }
                    }
                }
            )");

            TResponseBodyBuilder responseBody{modifierBody};
            TModifierAnalyticsInfoBuilder analyticsInfo;
            auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
            UNIT_ASSERT(!result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(),
                                     "<speaker is_whisper=\"true\"> kek");
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetDirectives().size(), 1);
            const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
            UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: true
                IsSoundSetLevelDirectiveApplied: false
            )"));
        }
    }

    Y_UNIT_TEST(ApplyWithAnotherDirectives) {
        TMockModifierContext ctx;
        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    MultiroomSessionId: "lessmeaning"
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            const auto baseRequest = ParseProtoText<NMegamind::TModifierRequest_TBaseRequest>(R"(
                Interfaces {
                    TtsPlayPlaceholder: true
                }
            )");
            EXPECT_CALL(ctx, GetBaseRequest()).WillRepeatedly(ReturnRef(baseRequest));

            const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                    Directives {
                        ClearQueueDirective {
                            Name: "clear_queue"
                        }
                    }
                }
            )");

            TResponseBodyBuilder responseBody{modifierBody};
            TModifierAnalyticsInfoBuilder analyticsInfo;
            auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
            UNIT_ASSERT(!result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(),
                                     "sil<[100]> <speaker is_whisper=\"true\"> kek");
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetDirectives().size(), 3);
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[0].HasSoundSetLevelDirective());
            UNIT_ASSERT_MESSAGES_EQUAL(
                    responseBody.GetModifierBody().GetLayout().GetDirectives()[0].GetSoundSetLevelDirective(),
                    ParseProtoText<TSoundSetLevelDirective>(R"(
                        Name: "sound_set_level"
                        MultiroomSessionId: "lessmeaning"
                        NewLevel: 3
                    )"));
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[1].HasTtsPlayPlaceholderDirective());
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[2].HasClearQueueDirective());
            const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
            UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: true
                IsSoundSetLevelDirectiveApplied: true
            )"));
        }
    }

    Y_UNIT_TEST(ApplyWithAnotherDirectivesAndTtsPlaceholder) {
        TMockModifierContext ctx;
        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    MultiroomSessionId: "lessmeaning"
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            const auto baseRequest = ParseProtoText<NMegamind::TModifierRequest_TBaseRequest>(R"(
                Interfaces {
                    TtsPlayPlaceholder: true
                }
            )");
            EXPECT_CALL(ctx, GetBaseRequest()).WillRepeatedly(ReturnRef(baseRequest));

            const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                    Directives {
                        ClearQueueDirective {
                            Name: "clear_queue"
                        }
                    }
                    Directives {
                        TtsPlayPlaceholderDirective {
                            Name: "tts_play_placeholder"
                        }
                    }
                }
            )");

            TResponseBodyBuilder responseBody{modifierBody};
            TModifierAnalyticsInfoBuilder analyticsInfo;
            auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
            UNIT_ASSERT(!result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(),
                                     "sil<[100]> <speaker is_whisper=\"true\"> kek");
            UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetDirectives().size(), 3);
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[0].HasSoundSetLevelDirective());
            UNIT_ASSERT_MESSAGES_EQUAL(
                    responseBody.GetModifierBody().GetLayout().GetDirectives()[0].GetSoundSetLevelDirective(),
                    ParseProtoText<TSoundSetLevelDirective>(R"(
                        Name: "sound_set_level"
                        MultiroomSessionId: "lessmeaning"
                        NewLevel: 3
                    )"));
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[1].HasClearQueueDirective());
            UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[2].HasTtsPlayPlaceholderDirective());
            const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
            UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: true
                IsSoundSetLevelDirectiveApplied: true
            )"));
        }
    }

    Y_UNIT_TEST(ApplyWithoutWhisperTagDeprecated) {
        TMockModifierContext ctx;
        const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    MultiroomSessionId: "lessmeaning"
                    IsWhisperTagDisabled: true
                }
            )");
        EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
        const auto baseRequest = ParseProtoText<NMegamind::TModifierRequest_TBaseRequest>(R"(
                Interfaces {
                    TtsPlayPlaceholder: true
                }
            )");
        EXPECT_CALL(ctx, GetBaseRequest()).WillRepeatedly(ReturnRef(baseRequest));

        const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                    Directives {
                        ClearQueueDirective {
                            Name: "clear_queue"
                        }
                    }
                    Directives {
                        TtsPlayPlaceholderDirective {
                            Name: "tts_play_placeholder"
                        }
                    }
                }
            )");

        TResponseBodyBuilder responseBody{modifierBody};
        TModifierAnalyticsInfoBuilder analyticsInfo;
        auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
        UNIT_ASSERT(!result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(), "sil<[100]> kek");
        UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetDirectives().size(), 3);
        UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[0].HasSoundSetLevelDirective());
        UNIT_ASSERT_MESSAGES_EQUAL(
            responseBody.GetModifierBody().GetLayout().GetDirectives()[0].GetSoundSetLevelDirective(),
            ParseProtoText<TSoundSetLevelDirective>(R"(
                        Name: "sound_set_level"
                        MultiroomSessionId: "lessmeaning"
                        NewLevel: 3
                    )"));
        UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[1].HasClearQueueDirective());
        UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[2].HasTtsPlayPlaceholderDirective());
        const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
        UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
            IsWhisperTagApplied: false
            IsSoundSetLevelDirectiveApplied: true
        )"));
    }

    Y_UNIT_TEST(ApplyWithoutWhisperTag) {
        TMockModifierContext ctx;
        const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    MultiroomSessionId: "lessmeaning"
                    IsWhisperTagDisabled: true
                }
            )");
        EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
        const auto baseRequest = ParseProtoText<NMegamind::TModifierRequest_TBaseRequest>(R"(
                Interfaces {
                    TtsPlayPlaceholder: true
                }
            )");
        EXPECT_CALL(ctx, GetBaseRequest()).WillRepeatedly(ReturnRef(baseRequest));

        const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                    Directives {
                        ClearQueueDirective {
                            Name: "clear_queue"
                        }
                    }
                    Directives {
                        TtsPlayPlaceholderDirective {
                            Name: "tts_play_placeholder"
                        }
                    }
                }
            )");

        TResponseBodyBuilder responseBody{modifierBody};
        TModifierAnalyticsInfoBuilder analyticsInfo;
        auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
        UNIT_ASSERT(!result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(), "sil<[100]> kek");
        UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetDirectives().size(), 3);
        UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[0].HasSoundSetLevelDirective());
        UNIT_ASSERT_MESSAGES_EQUAL(
            responseBody.GetModifierBody().GetLayout().GetDirectives()[0].GetSoundSetLevelDirective(),
            ParseProtoText<TSoundSetLevelDirective>(R"(
                        Name: "sound_set_level"
                        MultiroomSessionId: "lessmeaning"
                        NewLevel: 3
                    )"));
        UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[1].HasClearQueueDirective());
        UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives()[2].HasTtsPlayPlaceholderDirective());
        const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
        UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
            IsWhisperTagApplied: false
            IsSoundSetLevelDirectiveApplied: true
        )"));
    }

    Y_UNIT_TEST(ApplyWithPreviousRequestWhisper) {
        TMockModifierContext ctx;
        const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                    MultiroomSessionId: "lessmeaning"
                    IsPreviousRequestWhisper: true
                }
            )");
        EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
        NMegamind::TModifierRequest_TBaseRequest baseRequest;
        EXPECT_CALL(ctx, GetBaseRequest()).WillRepeatedly(ReturnRef(baseRequest));

        const auto modifierBody = ParseProtoText<TModifierBody>(R"(
                Layout {
                    OutputSpeech: "kek"
                }
            )");

        TResponseBodyBuilder responseBody{modifierBody};
        TModifierAnalyticsInfoBuilder analyticsInfo;
        auto result = TryApplyImpl(ctx, responseBody, analyticsInfo);
        UNIT_ASSERT(!result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(responseBody.GetModifierBody().GetLayout().GetOutputSpeech(),
                                 "<speaker is_whisper=\"true\"> kek");
        UNIT_ASSERT(responseBody.GetModifierBody().GetLayout().GetDirectives().empty());
        const auto actualAnalytics = std::move(analyticsInfo).MoveProto().GetWhisper();
        UNIT_ASSERT_MESSAGES_EQUAL(actualAnalytics, ParseProtoText<NAlice::NModifiers::NWhisper::TWhisper>(R"(
                IsWhisperTagApplied: true
                IsSoundSetLevelDirectiveApplied: false
            )"));
    }

    Y_UNIT_TEST(ApplicabilityHint) {
        TMockModifierContext ctx;

        const auto modifierBody = ParseProtoText<TModifierBody>(R"(
            Layout {
                OutputSpeech: "kek"
            }
        )");

        TResponseBodyBuilder responseBody{modifierBody};

        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: false
                }
                ContextualData {
                    Whisper {
                        Hint: Default
                    }
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            UNIT_ASSERT(!IsApplicable(ctx, responseBody));
        }

        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                }
                ContextualData {
                    Whisper {
                        Hint: Default
                    }
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            UNIT_ASSERT(IsApplicable(ctx, responseBody));
        }

        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: false
                }
                ContextualData {
                    Whisper {
                        Hint: ForcedEnable
                    }
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            UNIT_ASSERT(IsApplicable(ctx, responseBody));
        }

        {
            const auto features = ParseProtoText<NMegamind::TModifierFeatures>(R"(
                SoundSettings {
                    IsWhisper: true
                }
                ContextualData {
                    Whisper {
                        Hint: ForcedDisable
                    }
                }
            )");
            EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));
            UNIT_ASSERT(!IsApplicable(ctx, responseBody));
        }
    }
}

} // namespace
