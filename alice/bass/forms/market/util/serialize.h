#pragma once

#include <alice/bass/forms/market/types.h>
#include <alice/bass/forms/market/types/model.h>
#include <alice/bass/forms/market/types/offer.h>
#include <alice/bass/forms/market/types/warning.h>
#include <alice/bass/forms/market/types/picture.h>
#include <alice/bass/forms/market/client/checkout.sc.h>

namespace NBASS {

namespace NMarket {

NSc::TValue SerializeModelPrices(const TModel& model);
NSc::TValue SerializeOfferPrices(const TOffer& offer);
NSc::TValue SerializeBlueOfferPrices(const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& offer);
void SerializeWarnings(const TVector<TWarning>& warnings, TWarningsScheme outputWarnings);
void SerializePicture(const TPicture& picture, NBassApi::TPicture<TBoolSchemeTraits> outputPicture);
void SerializePicture(const TPicture& picture, NSc::TValue& outputData);

} // namespace NMarket

} // namespace NBASS
