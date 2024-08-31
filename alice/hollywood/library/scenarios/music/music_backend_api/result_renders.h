#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/biometry/process_biometry.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/fast_data.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/callback_payload.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/requests_helper/requests_helper.h>
#include <alice/hollywood/library/scenarios/music/show_view_builder/show_view_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

#include <alice/library/music/defs.h>

#include <memory>

namespace NAlice::NHollywood::NMusic {

NJson::TJsonValue MakeMusicAnswer(const TQueueItem& currentItem, const TContentId& contentId);

namespace NImpl { // TODO(vitvlkv): This should not be a NImpl namespace! Refactor

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderEmptyResponseHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                             const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                             TNlgWrapper& nlgWrapper, IRng& rng,
                                             const TShowViewBuilderSources sources,
                                             const TMusicShotsFastData& shots);

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderGenerativeHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                          const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                          TNlgWrapper& nlgWrapper);

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                const TMaybeRawHttpResponse& response,
                                const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                TNlgWrapper& nlgWrapper, IRng& rng, TResponseBodyBuilder::TRenderData& renderData,
                                const TShowViewBuilderSources sources,
                                const TMusicShotsFastData& shots);

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ContinueThinClientRenderHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                   const TMaybeRawHttpResponse& response,
                                   const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                   TNlgWrapper& nlgWrapper, IRng& rng, const TMusicFastData* fastData,
                                   TResponseBodyBuilder::TRenderData& renderData,
                                   const TShowViewBuilderSources sources,
                                   const TMusicShotsFastData& shots);

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderNonPremiumHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                          const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                          TNlgWrapper& nlgWrapper);

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderUnauthorizedHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                            const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                            TNlgWrapper& nlgWrapper);

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderErrorHandleImpl(TScenarioHandleContext& ctx, const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                     TNlgWrapper& nlgWrapper, const TMusicContext& mCtx);

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThinClientRenderPromoHandleImpl(TScenarioHandleContext& ctx, TMusicContext& mCtx,
                                     const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                     TNlgWrapper& nlgWrapper);

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse>
RunThinClientRenderGenerativeUnsupportedPlayerCommand(
    const NMusic::TScenarioState& scState,
    TScenarioHandleContext& ctx,
    const NHollywood::TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TStringBuf nlgTemplateName,
    TMusicArguments_EPlayerCommand unsupportedPlayerCommand);

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse>
RunThinClientRenderFmRadioUnsupportedPlayerCommand(
    const TMusicQueueWrapper& mq,
    const NMusic::TScenarioState& scState,
    TScenarioHandleContext& ctx,
    const NHollywood::TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TStringBuf nlgTemplateName,
    TMusicArguments_EPlayerCommand unsupportedPlayerCommand);

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse>
RunThinClientRenderUnknownMusicPlayerCommand(
    TScenarioHandleContext& ctx,
    const NHollywood::TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TStringBuf nlgTemplateName);

void RenderLikeNlgResult(TScenarioHandleContext& ctx,
                         const NHollywood::TScenarioRunRequestWrapper& runRequest, const TBiometryData& biometryData,
                         THwFrameworkRunResponseBuilder& builder);

void RenderDislikeNlgResult(TScenarioHandleContext& ctx,
                            const NHollywood::TScenarioApplyRequestWrapper& applyRequest, const TBiometryData& biometryData,
                            TApplyResponseBuilder& builder, NMusic::TScenarioState& scState, bool isGenerative = false);

void RenderTimestampSkipNlgResult(TScenarioHandleContext& ctx,
                                  const NHollywood::TScenarioApplyRequestWrapper& applyRequest, TApplyResponseBuilder& builder);

template <typename TScenarioRequestWrapper, typename TResponseBuilder>
void RenderShuffleNlgResult(TScenarioHandleContext& ctx, const TScenarioRequestWrapper& request,
                            TResponseBuilder& builder, const bool isRadio)
{
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering shuffle render_result NLG...";
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TNlgData nlgData{logger, request};
    nlgData.Context["is_radio"] = isRadio;
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);

    if (const auto frameProto = request.Input().FindSemanticFrame(::NAlice::NMusic::PLAYER_SHUFFLE)) {
        const auto shuffleCommandFrame = TFrame::FromProto(*frameProto);
        if (auto slot = shuffleCommandFrame.FindSlot(::NAlice::NMusic::SLOT_DISABLE_NLG)) {
            if (auto maybeValue = slot->Value.template As<bool>()) {
                nlgData.Context["nlg_disabled"] = *maybeValue;
            }
        }
    }

    LOG_INFO(logger) << "Constructed nlg context=" << nlgData.Context;

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_PLAYER_SHUFFLE, "render_result", {}, nlgData);
}

void RenderNoPrevTrackNlgResult(TScenarioHandleContext& ctx, const NHollywood::TScenarioRunRequestWrapper& request,
                                THwFrameworkRunResponseBuilder& builder);

template <typename TScenarioRequestWrapper, typename TResponseBuilder>
void RenderShotsLikeDislikeFeedback(TScenarioHandleContext& ctx, const TScenarioRequestWrapper& request,
                                    const TBiometryData& biometryData, TResponseBuilder& builder, bool isLike) {
    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Rendering shots " << (isLike ? "like" : "dislike") << " render_result NLG...";
    auto& bodyBuilder = builder.GetResponseBodyBuilder() ? *builder.GetResponseBodyBuilder()
                                                         : builder.CreateResponseBodyBuilder();

    TNlgData nlgData{logger, request};
    nlgData.Context["attentions"]["biometry_guest"] = biometryData.IsIncognitoUser;
    nlgData.Context["is_shot"] = true;
    nlgData.Context["user_name"] = biometryData.OwnerName;
    LOG_INFO(logger) << "Constructed nlg context=" << nlgData.Context;

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(
        isLike ? TEMPLATE_PLAYER_LIKE : TEMPLATE_PLAYER_DISLIKE, "render_result", {}, nlgData);
}

} // namespace NImpl

} // namespace NAlice::NHollywood::NMusic
