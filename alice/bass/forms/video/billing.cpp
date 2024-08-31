#include "billing.h"

#include "utils.h"
#include "video_provider.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/library/music/defs.h>

#include <util/string/join.h>
#include <util/system/compiler.h>
#include <util/system/yassert.h>

namespace NBASS::NVideo {

namespace {

constexpr TStringBuf FIELD_STATUS = "status";
constexpr TStringBuf FIELD_PAYLOAD = "payload";
constexpr TStringBuf FIELD_PROVIDERS = "providers";
constexpr TStringBuf FIELD_PROVIDERS_TO_LOGIN = "providers_to_login";
constexpr TStringBuf FIELD_REJECTION_REASON = "provider_rejection_reasons";
constexpr TStringBuf FIELD_URL = "url";
constexpr TStringBuf FIELD_PERSONAL_CARD = "personal_card";

constexpr TStringBuf STATUS_AVAILABLE = "available";
constexpr TStringBuf STATUS_PROVIDER_LOGIN_REQUIRED = "provider_login_required";
constexpr TStringBuf STATUS_PAYMENT_REQUIRED = "payment_required";

class TVideoError {
public:
    TVideoError() = default;
    TVideoError(const TVideoError& rhs)
    {
        Msg << rhs.Msg;
    }

    TVideoError(TVideoError&& rhs) = default;

    operator TResultValue() const {
        return TError{TError::EType::VIDEOERROR, Msg};
    }

    template <typename TValue>
    TVideoError& operator<<(const TValue& value) {
        Msg << value;
        return *this;
    }

private:
    TStringBuilder Msg;
};

bool IsSupportedType(TStringBuf type) {
    return ToBillingType(type).Defined();
}

bool IsSupportedProvider(TStringBuf providerName) {
    return providerName == NVideoCommon::PROVIDER_IVI || providerName == NVideoCommon::PROVIDER_AMEDIATEKA ||
           providerName == NVideoCommon::PROVIDER_KINOPOISK;
}

bool FillProviderItem(TLightVideoItemConstScheme item, EBillingType type, NSc::TValue& contentItem) {
    if (!IsSupportedProvider(item.ProviderName()))
        return false;

    NSc::TValue providerItem;

    bool valid = true;

    auto setId = [&providerItem, &valid](TStringBuf key, TStringBuf value) {
        if (value.empty()) {
            LOG(ERR) << "Value for " << key << " is empty." << Endl;
            valid = false;
            return;
        }

        providerItem[key] = value;
    };

    switch (type) {
        case EBillingType::Episode:
            setId("id", item.ProviderItemId());

            // For some episodes season id may be empty.
            if (!item.TvShowSeasonId()->empty())
                setId("season_id", item.TvShowSeasonId());

            setId("tv_show_id", item.TvShowItemId());
            break;
        case EBillingType::Season:
            setId("tv_show_id", item.TvShowItemId());
            if (!item.TvShowSeasonId()->empty())
                setId("id", item.TvShowSeasonId());
            break;
        case EBillingType::Film:
            setId("id", item.ProviderItemId());
            break;
    }

    if (!valid) {
        LOG(ERR) << "Failed to prepare provider item from: " << item->GetRawValue()->ToJson() << Endl;
        return false;
    }

    contentItem[item.ProviderName()] = providerItem;

    return true;
}

TResultValue LogAndReturnError(const NHttpFetcher::TResponse& response) {
    Y_ASSERT(response.IsError());

    TStringBuilder msg;
    msg << "Error requesting billing api: " << response.GetErrorText();
    if (!response.Data.empty())
        msg << ", data: " << response.Data;
    LOG(ERR) << msg << Endl;

    if (response.Code == 401 /* unauthorized */)
        return TError{TError::EType::UNAUTHORIZED, msg};
    return TError{TError::EType::VIDEOERROR, msg};
}

TMaybe<TVideoError> TryParsePlayData(const TStringBuf providerName, const NSc::TValue& data,
                                     const TString& httpResponse, TPlayData& result) {
    if (!data.IsDict())
        return TVideoError() << providerName << " item is not a dict: " << httpResponse;

    NSc::TValue payload = data[FIELD_PAYLOAD];

    auto dataUrl = data[FIELD_URL];
    if (!dataUrl.IsString())
        return TVideoError() << providerName << " url is not a string: " << httpResponse;

    TString url = TString{dataUrl.GetString()};

    TMaybe<TString> sessionToken;
    if (NSc::TValue sessionValue = payload.TrySelect("session"); sessionValue.IsString())
        sessionToken = TString{sessionValue.GetString()};

    result = {TString{providerName}, payload, url, sessionToken};
    return Nothing();
}

TMaybe<TVideoError> TryParseErrorData(TStringBuf providerName, TStringBuf data, const TString& httpResponse,
                                      TBillingError& result) {
    const auto parsedError = NVideoCommon::ParseRejectionReason(data);
    if (!parsedError)
        return TVideoError() << providerName << " error item is not a known error: " << httpResponse;

    result = {TString{providerName}, *parsedError};
    return Nothing();
}

template <typename TPayload>
TResultValue RequestContentImpl(IBillingAPI& api, const TRequestContentOptions& options,
                                const NSc::TValue& contentItem, const TPayload& contentPlayPayload,
                                TContentRequestResponse& response) {
    auto handle = api.RequestContent(options, contentItem, contentPlayPayload);
    Y_ASSERT(handle);

    NHttpFetcher::TResponse::TRef httpResponse = handle->Wait();
    Y_ASSERT(httpResponse);
    if (httpResponse->IsError())
        return LogAndReturnError(*httpResponse);

    NSc::TValue data;
    if (!NSc::TValue::FromJson(data, httpResponse->Data))
        return TVideoError() << "Failed to parse billing response: " << httpResponse->Data;

    if (!data[FIELD_STATUS].IsString())
        return TVideoError() << "No " << FIELD_STATUS << " field in billing response: " << httpResponse->Data;

    LOG(DEBUG) << "Billing response: " << httpResponse->Data << Endl;

    const auto status = data[FIELD_STATUS];

    if (status == STATUS_AVAILABLE) {
        const auto& providers = data[FIELD_PROVIDERS];
        if (!providers.IsDict())
            return TVideoError() << FIELD_PROVIDERS << " is not a dict: " << httpResponse->Data;

        switch (options.Type) {
            case TRequestContentOptions::EType::Play: {
                TPlayDataPayload playDataPayload;
                for (const auto& [ providerName, value ] : providers.GetDict()) {
                    TPlayData playData;
                    if (const auto error = TryParsePlayData(providerName, value, httpResponse->Data, playData))
                        return *error;
                    playDataPayload.push_back(std::move(playData));
                }

                if (playDataPayload.empty())
                    return TVideoError() << "Empty play data dict in billing response: " << httpResponse->Data;

                response =
                    TContentRequestResponse{TContentRequestResponse::EStatus::Available, std::move(playDataPayload)};
                return Nothing();
            }
            case TRequestContentOptions::EType::Buy: {
                TVector<TString> providerNames;
                for (const auto& [ providerName, value ] : providers.GetDict())
                    providerNames.push_back(TString{providerName});
                if (providerNames.empty())
                    return TVideoError() << "Empty list of providers in billing response: " << httpResponse->Data;

                response =
                    TContentRequestResponse{TContentRequestResponse::EStatus::Available, std::move(providerNames)};
                return Nothing();
            }
        };

        Y_UNREACHABLE();
        return TVideoError() << "Unsupported request content option: " << options.Type;
    }

    if (status == STATUS_PROVIDER_LOGIN_REQUIRED) {
        const auto& providers = data[FIELD_PROVIDERS_TO_LOGIN];
        if (!providers.IsArray())
            return TVideoError() << FIELD_PROVIDERS_TO_LOGIN << " is not an array: " << httpResponse->Data;

        TProviderNamePayload providerNames;
        for (const auto& item : providers.GetArray()) {
            if (!item.IsString())
                return TVideoError() << item << " is not a string: " << httpResponse->Data;
            providerNames.push_back(TString{item.GetString()});
        }

        if (providerNames.empty())
            return TVideoError() << "Empty error dict in billing response: " << httpResponse->Data;

        response =
            TContentRequestResponse{
                TContentRequestResponse::EStatus::ProviderLoginRequired,
                std::move(providerNames),
                std::move(data[FIELD_PERSONAL_CARD])
            };
        return Nothing();
    }

    if (status == STATUS_PAYMENT_REQUIRED) {
        const NSc::TValue& rejectionReasons = data[FIELD_REJECTION_REASON];
        if (!rejectionReasons.IsDict())
            return TVideoError() << FIELD_REJECTION_REASON << " field is not a dictionary: " << httpResponse->Data;

        TErrorPayload errorPayload;
        for (const auto& [providerName, errorKind] : rejectionReasons.GetDict()) {
            if (!errorKind.IsString())
                return TVideoError() << providerName << " error field is not a string: " << httpResponse->Data;

            TBillingError errorData;
            if (const auto error = TryParseErrorData(providerName, errorKind, httpResponse->Data, errorData))
                return *error;

            errorPayload.push_back(std::move(errorData));
        }

        if (errorPayload.empty())
            return TVideoError() << "Empty rejection reason set: " << httpResponse->Data;

        response = TContentRequestResponse{
            TContentRequestResponse::EStatus::PaymentRequired,
            std::move(errorPayload),
            std::move(data[FIELD_PERSONAL_CARD])
        };
        return Nothing();
    }

    return TVideoError() << "Unknown billing status: " << status;
}

bool HasPurchaseNotFoundError(const TContentRequestResponse& billingResponse) {
    struct TVisitor {
        bool operator()(const TPlayDataPayload& /* playPayload */) {
            return false;
        }

        bool operator()(const TProviderNamePayload& /* namePayload */) {
            return false;
        }

        bool operator()(const TErrorPayload& errorPayload) {
            return AnyOf(errorPayload, [](const TBillingError& data) {
                return data.Error == NVideoCommon::EPlayError::PURCHASE_NOT_FOUND ||
                       data.Error == NVideoCommon::EPlayError::SUBSCRIPTION_NOT_FOUND;
            });
        }
    };

    return std::visit(TVisitor{}, billingResponse);
}

template <typename T>
IOutputStream& PrintContainer(IOutputStream& os, const T& container) {
    bool first = true;
    for (const auto& value : container) {
        os << (first ? "" : ", ") << value;
        first = false;
    }
    return os;
}

} // namespace

NAlice::NBilling::TPromoAvailability GetPlusPromoAvailability(TContext& ctx, bool needActivatePromoUri) {
    TBillingAPI api(ctx);

    NHttpFetcher::THandle::TRef handle = api.GetPlusPromoAvailability();
    NHttpFetcher::TResponse::TRef resp = handle->Wait();
    if (resp->IsError()) {
        return NAlice::NBilling::TPromoAvailability{false /* IsAvailable */, "" /* ActivatePromoUri */, "" /* ExtraPeriodExpiresDate */};
    }

    LOG(INFO) << "Quasar billing response: " << resp->Data << Endl;
    const auto& billingResponse = NAlice::NBilling::ParseBillingResponse(resp->Data, needActivatePromoUri);

    NAlice::NBilling::TPromoAvailability promoAvailability;
    if (const auto error = std::get_if<NAlice::NBilling::TBillingError>(&billingResponse)) {
        LOG(ERR) << "Billing response parsing error. " << error->Message() << Endl;
    } else {
        promoAvailability = std::get<NAlice::NBilling::TPromoAvailability>(billingResponse);
    }
    return promoAvailability;
}

TMaybe<EBillingType> ToBillingType(TStringBuf type) {
    if (type == ToString(EItemType::TvShowEpisode))
        return EBillingType::Episode;
    if (type == ToString(EItemType::Movie))
        return EBillingType::Film;
    return Nothing();
}

bool SupportedForBilling(const TLightVideoItem& item) {
    return SupportedForBilling(TLightVideoItemConstScheme(&item.Value()));
}

bool SupportedForBilling(TLightVideoItemConstScheme item) {
    if (!IsSupportedProvider(item.ProviderName())) {
        LOG(WARNING) << "Not supported provider: " << item.ProviderName() << Endl;
        return false;
    }

    if (!IsSupportedType(item.Type())) {
        LOG(WARNING) << "Not supported type: " << item.Type() << Endl;
        return false;
    }

    return true;
}

bool FillContentItemFromItem(TLightVideoItemConstScheme item, NSc::TValue& contentItem) {
    if (!SupportedForBilling(item))
        return false;

    const auto billingType = ToBillingType(item.Type());
    Y_ASSERT(billingType.Defined());
    return FillContentItemFromItem(item, *billingType, contentItem);
}

bool FillContentItemFromItem(TLightVideoItemConstScheme item, EBillingType type, NSc::TValue& contentItem) {
    if (!FillProviderItem(item, type, contentItem))
        return false;
    contentItem["type"] = ToString(type);
    return true;
}

bool FillContentItemFromProvidersInfo(TVideoItemConstScheme item, EBillingType type, NSc::TValue& contentItem) {
    NSc::TValue ci;

    bool hasProviderInfo = false;
    ForEachProviderInfo(item, [&type, &ci, &hasProviderInfo](TLightVideoItemConstScheme providerInfo) {
        if (!SupportedForBilling(providerInfo))
            return;
        if (FillProviderItem(providerInfo, type, ci))
            hasProviderInfo = true;
    });

    if (!hasProviderInfo)
        return false;

    ci["type"] = ToString(type);
    contentItem.MergeUpdate(ci);

    return true;
}

bool FillContentItemForSeason(TStringBuf providerName, const TSeasonDescriptor& season, NSc::TValue& contentItem) {
    if (!IsSupportedProvider(providerName))
        return false;

    TLightVideoItem item;
    item->ProviderName() = providerName;
    item->TvShowItemId() = season.SerialId;
    if (season.Id)
        item->TvShowSeasonId() = *season.Id;

    return FillContentItemFromItem(item.Scheme(), EBillingType::Season, contentItem);
}

bool FillContentItemForSeason(TLightVideoItemConstScheme episode, NSc::TValue& contentItem) {
    if (!FillProviderItem(episode, EBillingType::Season, contentItem))
        return false;
    contentItem["type"] = ToString(EBillingType::Season);
    return true;
}

TResultValue RequestContent(TContext& ctx, const TRequestContentOptions& options, const NSc::TValue& contentItem,
                            TRequestContentPayloadConstScheme contentPlayPayload, TContentRequestResponse& response) {
    TBillingAPI api(ctx);
    return RequestContent(api, options, contentItem, contentPlayPayload, response);
}

TResultValue RequestContent(IBillingAPI& api, const TRequestContentOptions& options, const NSc::TValue& contentItem,
                            TRequestContentPayloadConstScheme contentPlayPayload, TContentRequestResponse& response) {
    return RequestContentImpl(api, options, contentItem, contentPlayPayload, response);
}

TResultValue RequestContent(TContext& ctx, const TRequestContentOptions& options, const NSc::TValue& contentItem,
                            TShowPayScreenCommandDataConstScheme contentPlayPayload,
                            TContentRequestResponse& response) {
    TBillingAPI api(ctx);
    return RequestContent(api, options, contentItem, contentPlayPayload, response);
}

TResultValue RequestContent(IBillingAPI& api, const TRequestContentOptions& options, const NSc::TValue& contentItem,
                            TShowPayScreenCommandDataConstScheme contentPlayPayload,
                            TContentRequestResponse& response) {
    return RequestContentImpl(api, options, contentItem, contentPlayPayload, response);
}

TResultValue AddAttentionForPaymentRequired(TContext& ctx, const TContentRequestResponse& response,
                                            const TShowPayScreenCommandData& commandData) {
    if (HasPurchaseNotFoundError(response)) {
        if (IsTvOrModuleRequest(ctx)) {
            LOG(INFO) << "TV should not send pay push. Instead TV do buyings by remote" << Endl; // SMARTTVBACKEND-918
            ctx.AddAttention(NVideo::ATTENTION_TV_PAYMENT_WITHOUT_PUSH);
            ctx.AddStopListeningBlock();
        } else {
            AddShowPayPushScreenCommand(ctx, commandData);
            ctx.AddAttention(ATTENTION_SEND_PAY_PUSH_DONE);
        }
        return ResultSuccess();
    }

    const auto* errorPayload = std::get_if<TErrorPayload>(&response);
    Y_ENSURE(errorPayload && !errorPayload->empty(), "Invalid billing error response!");
    for (const TBillingError& errorData : *errorPayload)
        if (AddAttentionForPlayError(ctx, errorData.Error))
            return ResultSuccess();

    TString errMsg = TStringBuilder{} << "Unsupported billing error kinds in response: " << response;
    LOG(ERR) << errMsg << Endl;
    return TError{TError::EType::VIDEOERROR, errMsg};
}

} // namespace NBASS::NVideo

template <>
void Out<NBASS::NVideo::TPlayData>(IOutputStream& os, const NBASS::NVideo::TPlayData& playData) {
    os << "TPlayData [ProviderName: " << playData.ProviderName << ", Payload: " << playData.Payload
       << ", Url: " << playData.Url << "]";
}

template <>
void Out<NBASS::NVideo::TBillingError>(IOutputStream& os, const NBASS::NVideo::TBillingError& billingError) {
    os << "TBillingError [ProviderName: " << billingError.ProviderName << ", Error: " << billingError.Error << "]";
}

template <>
void Out<NBASS::NVideo::TContentRequestResponse>(IOutputStream& os,
                                                 const NBASS::NVideo::TContentRequestResponse& response) {
    os << "TContentRequestResponse [";
    os << "Status: " << response.Status << ", ";

    struct TPayloadPrinter {
        void operator()(const NBASS::NVideo::TPlayDataPayload& playPayload) {
            OS << "TPlayDataPayload [";
            NBASS::NVideo::PrintContainer(OS, playPayload);
            OS << "]";
        }

        void operator()(const NBASS::NVideo::TErrorPayload& errorPayload) {
            OS << "TErrorPayload [";
            NBASS::NVideo::PrintContainer(OS, errorPayload);
            OS << "]";
        }

        void operator()(const NBASS::NVideo::TProviderNamePayload& namePayload) {
            OS << "TProviderNamePayload [";
            NBASS::NVideo::PrintContainer(OS, namePayload);
            OS << "]";
        }

        IOutputStream& OS;
    };

    std::visit(TPayloadPrinter{os}, response);
    os << "]";
}
