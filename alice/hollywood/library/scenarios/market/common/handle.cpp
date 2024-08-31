#include "handle.h"

#include <alice/library/geo/protos/user_location.pb.h>

namespace NAlice::NHollywood::NMarket {

NGeobase::TId TDeprecatedMarketRunHandleImplBase::GetUserRegionId() const
{
    // Проверяю в самом начале, чтоб не пропустить случайно ситуацию,
    // когда тестровали на устройстве с явно заданным регионом
    // и не убедились, что DataSource USER_LOCATION приходит
    const auto* userLocationPtr =
        RequestWrapper().GetDataSource(NAlice::EDataSourceType::USER_LOCATION);
    Y_ENSURE(userLocationPtr, "USER_LOCATION resource is required");

    const auto& baseRequestProto = RequestWrapper().BaseRequestProto();
    if (baseRequestProto.HasOptions()) {
        if (const auto regionId = baseRequestProto.GetOptions().GetUserDefinedRegionId()) {
            return regionId;
        }
    }
    return userLocationPtr->GetUserLocation().GetUserRegion();
}

void TDeprecatedMarketRunHandleImplBase::SetIrrelevantResponse(TRunResponseBuilder& builder)
{
    builder.SetIrrelevant();

    // Irrelvant response is not 100% guarantee that scenario won't be choosen.
    // So we need to retrun smth in response body.
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{Logger(), RequestWrapper()};
    bodyBuilder.AddRenderedText(
        TStringBuf("common"),
        TStringBuf("render_irrelevant"),
        nlgData
    );
}

void TDeprecatedMarketRunHandleImplBase::AddIrrelevantResponse()
{
    TRunResponseBuilder builder(&NlgWrapper());
    SetIrrelevantResponse(builder);
    AddResponse(std::move(builder));
}

} // namespace NAlice::NHollywood::NMarket
