#include "search_info.h"

namespace NAlice::NHollywood::NMarket {

TSearchInfo::TSearchInfo(TStringBuf text, NGeobase::TId regionId)
    : Text(ToString(text))
    , RegionId(regionId)
{}

TSearchInfo::TSearchInfo(const NProto::TSearchInfo& proto)
{
    Text = proto.GetText();
    if (const auto regionId = proto.GetRegionId()) {
        RegionId = regionId;
    }
    if (proto.HasCategory()) {
        Category.ConstructInPlace(proto.GetCategory());
    }
    GlFilters = TCgiGlFilters(proto.GetGlFilters());
    RedirectParams = TCgiRedirectParameters(proto.GetRedirectParams());
    if (proto.GetGoodState()) {
        GoodState = FromString<EMarketGoodState>(proto.GetGoodState());
    }
}

NProto::TSearchInfo TSearchInfo::ToProto() const
{
    NProto::TSearchInfo proto;
    *proto.MutableText() = Text;
    proto.SetRegionId(RegionId);
    if (Category) {
        *proto.MutableCategory() = Category->ToProto();
    }
    proto.SetRedirectParams(RedirectParams.Print());
    *proto.MutableGlFilters() = GlFilters.ToProto();
    if (GoodState) {
        *proto.MutableGoodState() = ToString(*GoodState);
    }

    return proto;
}

TString TSearchInfo::CreateMarketUrl(const TMarketUrlBuilder& urlBuilder) const {
    if (Category.Defined()) {
        return urlBuilder.GetMarketCategoryUrl(
            Category.GetRef(),
            GlFilters,
            RegionId,
            GoodState,
            Text,
            RedirectParams
        );
    }
    return urlBuilder.GetMarketSearchUrl(
        Text,
        RegionId,
        GoodState,
        false
    );
}

TReportPrimeRequest TSearchInfo::CreateReportRequest(TReportClient& client) const
{
    TReportPrimeRequest request = CreateBaseReportRequest(client);
    request.SetRegionId(RegionId);
    request.SetGlFilters(GlFilters);
    request.SetRedirectParams(RedirectParams);
    return request;
}

TReportPrimeRequest TSearchInfo::CreateBaseReportRequest(TReportClient& client) const
{
    if (Category.Defined()) {
        return client.CreateRequest(Category.GetRef(), Text);
    }
    return client.CreateRequest(Text);
}

void TSearchInfo::ApplyRedirect(const TReportPrimeRedirect& redirect)
{
    Text = redirect.Text.GetOrElse(TString{});
    if (redirect.RegionId) {
        RegionId = *redirect.RegionId;
    }
    if (redirect.Category) {
        Category = *redirect.Category;
    }
    for (const auto& [id, values] : redirect.GlFilters) {
        GlFilters[id] = values;
    }
    redirect.RedirectParams.AddToCgi(RedirectParams);
}

} // namespace NAlice::NHollywood::NMarket
