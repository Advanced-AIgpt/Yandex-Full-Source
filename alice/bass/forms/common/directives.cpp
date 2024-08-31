#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/context/context.h>
#include <alice/protos/analytics/dummy_response/response.pb.h>


namespace NBASS {

bool TryAddOpenUriDirective(TContext& ctx, const TString& uri, const TStringBuf screen_id) {
    if (!ctx.ClientFeatures().SupportsOpenLink()) {
        return false;
    }
    Y_ENSURE(!uri.empty(), "open_uri directive requies a non-empty uri");    
    NSc::TValue intentData;
    intentData["uri"].SetString(uri);
    if (!screen_id.empty()) { 
        intentData["screen_id"].SetString(TStringBuf("cloud_ui"));
    }
    ctx.AddCommand<TOpenUriDirective>(TStringBuf("open_uri"), intentData);
    return true;
}

bool TryAddShowPromoDirective(TContext& ctx) {
    if (!ctx.ClientFeatures().SupportsShowPromo()) {
        return false;
    }

    NAlice::NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder = ctx.GetAnalyticsInfoBuilder();
    NAlice::NScenarios::TAnalyticsInfo::TObject source;
    source.MutableDummyResponse()->SetReason(NAlice::NAnalytics::NDummyResponse::TResponse_EReason_SurfaceInability);
    analyticsInfoBuilder.AddObject(source);

    ctx.AddCommand<TShowPromoDirective>(TStringBuf("show_promo"), NSc::TValue{});
    return true;
}

}
