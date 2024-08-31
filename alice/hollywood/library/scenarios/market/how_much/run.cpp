#include "run.h"

#include "renderer.h"
#include <alice/hollywood/library/scenarios/market/how_much/proto/apply_arguments.pb.h>

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

namespace NAlice::NHollywood::NMarket::NHowMuch {

TRunImpl::TRunImpl(TMarketRunContext& ctx)
    : Ctx(ctx)
    , Scenario(ctx)
    , GeoSupport(ctx.Geobase(), ctx.Logger())
{}

void TRunImpl::Do()
{
    TRunResponseBuilder builder(&Ctx.NlgWrapper());

    const auto frame = Ctx.RequestWrapper().Input().FindSemanticFrame(Scenario.FrameName);
    if (!frame) {
        LOG_INFO(Ctx.Logger()) << "No appropriate frame found for " << Scenario.ScenarioName;
        Ctx.SetIrrelevantResponse(builder);
        Scenario.SetAnalyticsInfo(builder.GetResponseBodyBuilder()->CreateAnalyticsInfoBuilder());
        Ctx.AddResponse(std::move(builder));
        return;
    }
    const TString requestText = GetRequestSlotValue(*frame);
    const auto regionId = Ctx.GetUserRegionId();

    if (!regionId || !GeoSupport.IsMarketSupportedForGeoId(*regionId)) {
        Ctx.SetIrrelevantResponse(builder);
        Scenario.SetAnalyticsInfo(builder.GetResponseBodyBuilder()->CreateAnalyticsInfoBuilder());
        Ctx.AddResponse(std::move(builder));
        return;
    }

    if (requestText.empty()) {
        auto& bodyBuilder = builder.CreateResponseBodyBuilder();
        THowMuchRenderer renderer = MakeHowMuchRenderer(Ctx, bodyBuilder);
        renderer.RenderAskRequestSlot();
        Scenario.SetAnalyticsInfo(bodyBuilder.CreateAnalyticsInfoBuilder());

        Ctx.AddResponse(std::move(builder));
        return;
    }

    if (Ctx.FastData().ContainsVulgarQuery(requestText)) {
        LOG_INFO(Ctx.Logger()) << "Request contains forbidden words";
        Ctx.SetIrrelevantResponse(builder);
        Scenario.SetAnalyticsInfo(builder.GetResponseBodyBuilder()->CreateAnalyticsInfoBuilder());
        Ctx.AddResponse(std::move(builder));
        return;
    }

    // save apply args
    TApplyArguments applyArgs;
    *applyArgs.MutableRequestText() = requestText;
    applyArgs.SetRegionId(*regionId);
    builder.SetApplyArguments(applyArgs);
    Ctx.AddResponse(std::move(builder));
}

TString TRunImpl::GetRequestSlotValue(const NAlice::TSemanticFrame& frame)
{
    TStringBuilder result;
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetName() == TStringBuf("request")) {
            if (result) {
                result << ' ';
            }
            result << NNlu::TRequestNormalizer::Normalize(ELanguage::LANG_RUS, slot.GetValue());
        }
    }
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetName() == TStringBuf("request_end")) {
            result << ' ';
            result << NNlu::TRequestNormalizer::Normalize(ELanguage::LANG_RUS, slot.GetValue());
            break;
        }
    }
    return result;
}

} // namespace NAlice::NHollywood::NMarket::NHowMuch
