#include "tv_home_render.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/proto/proto.h>
#include <alice/library/json/json.h>


using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

void TTvHomeRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const NJson::TJsonValue response = RetireHttpResponseJson(ctx);
    TDirective directive;
    TString analiticsActionId;

    if (response.IsArray()) {
        auto* setSmartTvCategoriesDirective = directive.MutableSetSmartTvCategoriesDirective();
        for (const auto& item: response.GetArray()) {
            auto* category = setSmartTvCategoriesDirective->AddCategories();
            category->SetCategoryId(item["category_id"].GetString());
            category->SetRank(item["rank"].GetUInteger());
            category->SetIcon(item["icon"].GetString());
            category->SetTitle(item["title"].GetString());
        }
        analiticsActionId = "droideka_set_categories";
    } else {
        NJson::TJsonValue directiveJson;
        if (response.Has("carousel_id")) {
            directiveJson.InsertValue("tv_set_single_carousel_directive", response);
            analiticsActionId = "droideka_set_carousel";
        } else {
            directiveJson.InsertValue("tv_set_carousels_directive", response);
            analiticsActionId = "droideka_set_carousels";
        }
        directive = JsonToProto<TDirective>(directiveJson, true, true);
    }

    const NScenarios::TScenarioApplyRequest requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper applyRequestWrapper(requestProto, ctx.ServiceCtx);
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), applyRequestWrapper, ctx.Rng, ctx.UserLang);
    TContinueResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    bodyBuilder.AddDirective(std::move(directive));

    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::DROIDEKA);
    analyticsInfoBuilder.AddAction(analiticsActionId, analiticsActionId, "");

    ctx.ServiceCtx.AddProtobufItem(*(std::move(builder).BuildResponse()), RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood
