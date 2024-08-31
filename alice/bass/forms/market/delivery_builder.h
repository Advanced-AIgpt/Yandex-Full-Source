#pragma once

#include "client/report_client.h"
#include "types.h"

namespace NBASS {

namespace NMarket {

class TDeliveryBuilder {
public:
    static bool TryFillWhiteDelivery(
        const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& offer,
        NBassApi::TOutputDelivery<TBoolSchemeTraits> delivery);
    static void FillBlueDelivery(
        const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& blueOffer,
        NBassApi::TOutputDelivery<TBoolSchemeTraits> delivery);

};

} // namespace NMarket

} // namespace NBASS
