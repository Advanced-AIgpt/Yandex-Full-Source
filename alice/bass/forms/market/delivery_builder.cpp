#include "delivery_builder.h"

#include "dynamic_data.h"


namespace NBASS {

namespace NMarket {

bool TDeliveryBuilder::TryFillWhiteDelivery(
    const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& offer,
    NBassApi::TOutputDelivery<TBoolSchemeTraits> delivery)
{
    if (offer.Delivery().Options().Size() == 0) {
        return false;
    }

    const auto& deliveryOption = offer.Delivery().Options()[0];
    if (deliveryOption.HasDayFrom()) {
        // todo: MALISA-240 Покрыть карточку с беру заказом тестами.
        // Для этого нужно научиться замораживать метод Now(), либо высчитывать дату доставки на стороне винса.
        const auto now = Now();
        const auto deliveryTm = now + TDuration::Days(deliveryOption.DayFrom());
        delivery.Courier().Date() = deliveryTm.FormatLocalTime("%Y.%m.%d");
    }
    if (deliveryOption.HasPrice()) {
        delivery.Courier().Price().Value() = deliveryOption.Price().Value();
        delivery.Courier().Price().Currency() = deliveryOption.Price().Currency();
    }
    delivery.HasPickup() = offer.Delivery().HasPickup();
    return true;
}

void TDeliveryBuilder::FillBlueDelivery(
    const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& blueOffer,
    NBassApi::TOutputDelivery<TBoolSchemeTraits> delivery)
{
    bool baseFieldsAreFilled = TryFillWhiteDelivery(blueOffer, delivery);
    if (!baseFieldsAreFilled) {
        LOG(ERR) << "No delivery options in default offer for sku " << blueOffer.MarketSku() << Endl;
        return;
    }

    if (TDynamicDataFacade::IsFreeDeliveryDate()) {
        delivery.Courier().Price().Value() = "0";
    }
    if (auto* promo = TDynamicDataFacade::ParticipatesInVendorFreeDeliveryPromotion(blueOffer.Vendor().Id())) {
        delivery.Promotion().Description() = promo->Description;
    }
}

} // namespace NMarket

} // namespace NBASS
