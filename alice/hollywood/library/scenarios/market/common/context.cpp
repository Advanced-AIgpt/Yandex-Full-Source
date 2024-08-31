#include "context.h"

#include <alice/library/geo/protos/user_location.pb.h>

namespace NAlice::NHollywood::NMarket {

bool TMarketBaseContext::CanOpenUri() const
{
    if (HasExpFlag(EMarketExperiment::SCREENLESS)) {
        return false;
    }
    const auto& clientInfo = RequestWrapper().ClientInfo();
    return clientInfo.IsSearchApp()
        || clientInfo.IsYaBrowser()
        || clientInfo.IsYaLauncher();
}

bool TMarketBaseContext::SupportsDivCards() const
{
    if (HasExpFlag(EMarketExperiment::SCREENLESS)) {
        return false;
    }
    return RequestWrapper().BaseRequestProto().GetInterfaces().GetCanRenderDivCards();
}

TNlgWrapper& TMarketBaseContext::NlgWrapper()
{
    if (NlgWrapper_.Empty()) {
        NlgWrapper_.ConstructInPlace(TNlgWrapper::Create(Ctx->Ctx.Nlg(), RequestWrapper(), Ctx->Rng, Ctx->UserLang));
    }
    return NlgWrapper_.GetRef();
}

const TMarketFastData& TMarketBaseContext::FastData() const
{
    if (!FastDataPtr) {
        FastDataPtr = Ctx->Ctx.GlobalContext().FastData().GetFastData<TMarketFastData>();
    }
    return *FastDataPtr;
}

const NGeobase::TLookup& TMarketBaseContext::Geobase() const
{
    return Ctx->Ctx.GlobalContext().CommonResources().Resource<TGeobaseResource>().GeobaseLookup();
}

TUserLocation TMarketBaseContext::CreateUserLocation(NGeobase::TId regionId) const
{
    return {
        Geobase(),
        regionId,
        RequestWrapper().ClientInfo().Timezone
    };
}

const TScenarioRunRequestWrapper& TMarketRunContext::RequestWrapper() const
{
    if (RequestWrapper_.Defined()) {
        return RequestWrapper_.GetRef();
    }
    RequestProto = GetMaybeOnlyProto<TScenarioRunRequest>(Ctx->ServiceCtx, REQUEST_ITEM);
    return RequestWrapper_.ConstructInPlace(RequestProto.GetRef(), Ctx->ServiceCtx);
}

void TMarketRunContext::AddResponse(TRunResponseBuilder&& builder)
{
    auto response = std::move(builder).BuildResponse();
    Ctx->ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

TMaybe<NGeobase::TId> TMarketRunContext::GetUserRegionId() const
{
    // Проверяю в самом начале, чтоб не пропустить случайно ситуацию,
    // когда тестровали на устройстве с явно заданным регионом
    // и не убедились, что DataSource USER_LOCATION приходит
    const auto* userLocationPtr =
        RequestWrapper().GetDataSource(NAlice::EDataSourceType::USER_LOCATION);
    Y_ENSURE(userLocationPtr, "USER_LOCATION resource is required");
    LOG_INFO(Logger()) << "Region data source " << *userLocationPtr;

    NGeobase::TId result = userLocationPtr->GetUserLocation().GetUserRegion();
    const auto& baseRequestProto = RequestWrapper().BaseRequestProto();
    if (baseRequestProto.HasOptions()) {
        if (const auto regionId = baseRequestProto.GetOptions().GetUserDefinedRegionId()) {
            LOG_INFO(Logger()) << "Use user defined region " << regionId;
            result = regionId;
        }
    }
    if (result == NGeobase::UNKNOWN_REGION) {
        LOG_INFO(Logger()) << "Region is undefined";
        return Nothing();
    }
    return result;
}

void TMarketRunContext::SetIrrelevantResponse(TRunResponseBuilder& builder)
{
    builder.SetIrrelevant();

    // Irrelvant response is not 100% guarantee that scenario won't be choosen.
    // So we need to retrun smth in response body.
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{Logger(), RequestWrapper()};
    // TODO(bas1330) move to TCommonResponseBuilder
    bodyBuilder.AddRenderedText(
        TStringBuf("common"),
        TStringBuf("render_irrelevant"),
        nlgData
    );
}

const TScenarioApplyRequestWrapper& TMarketApplyContext::RequestWrapper() const
{
    if (RequestWrapper_.Defined()) {
        return RequestWrapper_.GetRef();
    }
    RequestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(Ctx->ServiceCtx, REQUEST_ITEM);
    return RequestWrapper_.ConstructInPlace(RequestProto.GetRef(), Ctx->ServiceCtx);
}

void TMarketApplyContext::AddResponse(TApplyResponseBuilder&& builder)
{
    auto response = std::move(builder).BuildResponse();
    Ctx->ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMarket
