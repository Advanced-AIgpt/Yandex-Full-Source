#pragma once

#include "billing_api.h"
#include "defs.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/libs/video_common/video.sc.h>

#include <alice/library/billing/billing.h>

#include <library/cpp/scheme/scheme.h>

#include <utility>

namespace NBASS {
namespace NVideo {
class IBillingAPI;

enum class EBillingType {
    Episode /* "episode" */,
    Season /* "season" */,
    Film /* "film" */,
};

TMaybe<EBillingType> ToBillingType(TStringBuf type);

bool SupportedForBilling(const TLightVideoItem& item);
bool SupportedForBilling(TLightVideoItemConstScheme item);

bool FillContentItemFromItem(TLightVideoItemConstScheme item, NSc::TValue& contentItem);
bool FillContentItemFromItem(TLightVideoItemConstScheme item, EBillingType type, NSc::TValue& contentItem);
bool FillContentItemFromProvidersInfo(TVideoItemConstScheme item, EBillingType type, NSc::TValue& contentItem);

bool FillContentItemForSeason(TStringBuf providerName, const TSeasonDescriptor& season, NSc::TValue& contentItem);
bool FillContentItemForSeason(TLightVideoItemConstScheme item, NSc::TValue& contentItem);

NAlice::NBilling::TPromoAvailability GetPlusPromoAvailability(TContext& ctx, bool needActivatePromoUri);

struct TPlayData {
    TString ProviderName;
    NSc::TValue Payload;
    TString Url;
    TMaybe<TString> SessionToken;

    bool operator==(const TPlayData& rhs) const {
        return ProviderName == rhs.ProviderName && Payload == rhs.Payload && Url == rhs.Url &&
               SessionToken == rhs.SessionToken;
    }
};
using TPlayDataPayload = TVector<TPlayData>;

struct TBillingError {
    TString ProviderName;
    NVideoCommon::EPlayError Error;

    bool operator==(const TBillingError& rhs) const {
        return ProviderName == rhs.ProviderName && Error == rhs.Error;
    }
};
using TErrorPayload = TVector<TBillingError>;

using TProviderName = TString;
using TProviderNamePayload = TVector<TString>;

using TBillingData = std::variant<TPlayDataPayload, TErrorPayload, TProviderNamePayload>;

struct TContentRequestResponse : public TBillingData {
    enum class EStatus {
        Available /* "available" */,
        ProviderLoginRequired /* "provider_login_required" */,
        PaymentRequired /* "payment_required" */
    };

    TContentRequestResponse() = default;

    template<typename T>
    TContentRequestResponse(EStatus status, T&& billingData)
        : TBillingData(std::forward<T>(billingData))
        , Status(status)
        , PersonalCard(NSc::Null())
    {
    }

    template<typename T>
    TContentRequestResponse(EStatus status, T&& billingData, NSc::TValue&& personalCard)
        : TBillingData(std::forward<T>(billingData))
        , Status(status)
        , PersonalCard(std::move(personalCard))
    {
    }

    EStatus Status = EStatus::Available;
    NSc::TValue PersonalCard;
};

TResultValue RequestContent(TContext& ctx, const TRequestContentOptions& options, const NSc::TValue& contentItem,
                            TRequestContentPayloadConstScheme contentPlayPayload, TContentRequestResponse& response);
TResultValue RequestContent(IBillingAPI& api, const TRequestContentOptions& options, const NSc::TValue& contentItem,
                            TRequestContentPayloadConstScheme contentPlayPayload, TContentRequestResponse& response);

TResultValue RequestContent(TContext& ctx, const TRequestContentOptions& options, const NSc::TValue& contentItem,
                            TShowPayScreenCommandDataConstScheme contentPlayPayload,
                            TContentRequestResponse& response);
TResultValue RequestContent(IBillingAPI& api, const TRequestContentOptions& options, const NSc::TValue& contentItem,
                            TShowPayScreenCommandDataConstScheme contentPlayPayload,
                            TContentRequestResponse& response);

TResultValue AddAttentionForPaymentRequired(TContext& ctx, const TContentRequestResponse& response,
                                            const TShowPayScreenCommandData& commandData);

} // namespace NVideo
} // namespace NBASS
