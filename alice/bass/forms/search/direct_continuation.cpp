#include "direct_continuation.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/network/headers.h>

using namespace NHttpFetcher;

using namespace std::literals;

namespace NBASS::NDirectGallery {

namespace {

TRequestOptions GetRequestOptions() {
    TRequestOptions requestOptions;
    requestOptions.Timeout = TDuration::MilliSeconds(300);
    requestOptions.MaxAttempts = 2;
    requestOptions.RetryPeriod = TDuration::MilliSeconds(150);
    return requestOptions;
}

} // namespace

TMaybe<TString> TBlackBoxUidProvider::GetUid(TContext& ctx) const {
    TPersonalDataHelper persData{ctx};
    TMaybe<TString> uid;
    if (TString tmpUid; persData.GetUid(tmpUid)) {
        uid = std::move(tmpUid);
    }
    return uid;
}

TDirectGalleryHitConfirmContinuation::TDirectGalleryHitConfirmContinuation(TContext::TPtr ctx, TString hitCounter,
                                                                           TString linkHead,
                                                                           TVector<TString> linkTails,
                                                                           std::unique_ptr<IRequestAPI> requestAPI,
                                                                           std::unique_ptr<IUidProvider> uidProvider)
    : IContinuation{EFinishStatus::NeedCommit}
    , Context{ctx}
    , RequestAPI{std::move(requestAPI)}
    , UidProvider{std::move(uidProvider)}
    , HitCounter{std::move(hitCounter)}
    , LinkHead{std::move(linkHead)}
    , LinkTails{std::move(linkTails)}
{
}

TMaybe<TDirectGalleryHitConfirmContinuation>
TDirectGalleryHitConfirmContinuation::FromJson(NSc::TValue value, TGlobalContextPtr globalContext, NSc::TValue meta,
                                               const TString& authHeader, const TString& appInfoHeader,
                                               const TString& fakeTimeHeader, const TMaybe<TString>& userTicketHeader,
                                               const NSc::TValue& configPatch) {
    const NSc::TValue state = NSc::TValue::FromJson(value["State"].GetString());
    TIntrusivePtr<TContext> context;
    if (TResultValue contextParseResult = FillContext(context, state, globalContext, meta, authHeader, appInfoHeader,
                                                      fakeTimeHeader, userTicketHeader, configPatch)) {
        LOG(ERR) << "Cannot deserialize context: " << contextParseResult->Msg << Endl;
        return Nothing();
    }
    TStringBuf hitCounter = state["hit_counter"];
    TStringBuf linkHead = state["link_head"];
    const auto& serializedTails = state["link_tails"].GetArray();
    TVector<TString> linkTails(Reserve(serializedTails.size()));
    for (const auto& tail : serializedTails) {
        linkTails.push_back(TString{tail.GetString()});
    }

    return MakeMaybe<TDirectGalleryHitConfirmContinuation>(context, TString{hitCounter}, TString{linkHead},
                                                           std::move(linkTails));
}

NSc::TValue TDirectGalleryHitConfirmContinuation::ToJsonImpl() const {
    NSc::TValue serializedData;
    serializedData["context"] = Context->TopLevelToJson(TContext::EJsonOut::DataSources);
    serializedData["hit_counter"] = HitCounter;
    serializedData["link_head"] = LinkHead;

    auto& tails = serializedData["link_tails"];
    for (const auto& tail : LinkTails) {
        tails.Push().SetString(tail);
    }

    return serializedData;
}

TResultValue TDirectGalleryHitConfirmContinuation::Apply() {
    auto multiRequest = RequestAPI->WeakMultiRequest();
    auto requestOptions = GetRequestOptions();
    TMaybe<TString> uid = UidProvider->GetUid(*Context);

    auto fetchWithUid = [&uid](TRequest& request) {
        if (uid) {
            request.AddHeader(NAlice::NNetwork::HEADER_COOKIE, "yandexuid="sv + *uid);
        }
        return request.Fetch();
    };

    TVector<THandle::TRef> requests(Reserve(LinkTails.size() + 1));
    requests.push_back(fetchWithUid(*multiRequest->AddRequest(HitCounter, requestOptions)));
    for (const auto& tail : LinkTails) {
        requests.push_back(fetchWithUid(*multiRequest->AddRequest(LinkHead + tail, requestOptions)));
    }

    bool hasErrors = false;
    auto onError = [&hasErrors](TStringBuf text) {
        LOG(ERR) << "BS confirmation responded with an error: " << text << Endl;
        Y_STATS_INC_COUNTER("bs_confirmation_fail");
        hasErrors = true;
    };

    TVector<THandle::TRef> redirectedRequests(Reserve(uid ? 0 : LinkTails.size() + 1));
    auto redirects = RequestAPI->WeakMultiRequest();
    for (auto request : requests) {
        auto response = request->Wait();
        if (response->Code == HTTP_FOUND) {
            const auto* locationHeader = response->Headers.FindHeader(NAlice::NNetwork::HEADER_LOCATION);
            Y_STATS_INC_COUNTER("bs_confirmation_redirect");
            if (!locationHeader) {
                onError("Got an invalid redirect for url");
                continue;
            }
            LOG(INFO) << "Confirmation redirect to: " << locationHeader->Value() << Endl;

            auto request = redirects->AddRequest(locationHeader->Value(), requestOptions);
            if (const auto* cookieHeader = response->Headers.FindHeader(NAlice::NNetwork::HEADER_SET_COOKIE)) {
                TStringBuf yuid = TStringBuf{cookieHeader->Value()}.Before(';');
                LOG(INFO) << "Got " << NAlice::NNetwork::HEADER_SET_COOKIE << " header for redirect" << Endl;
                request->AddHeader(NAlice::NNetwork::HEADER_COOKIE, yuid);
            }
            redirectedRequests.push_back(request->Fetch());

        } else if (!response->IsHttpOk()) {
            onError(response->GetErrorText()); // Redirect is considered as error as well.
        }
    }

    for (auto request : redirectedRequests) {
        auto response = request->Wait();
        if (!response->IsHttpOk()) { // We support only one level of indirection here.
            onError(response->GetErrorText());
        }
    }

    if (!hasErrors) {
        Y_STATS_INC_COUNTER("bs_confirmation_success");
    }
    return ResultSuccess();
}

void RegisterDirectGalleryContinuation(TContinuationParserRegistry& registry) {
    registry.Register<TDirectGalleryHitConfirmContinuation>(TDirectGalleryHitConfirmContinuation::NAME);
}

} // namespace NBASS::NDirectGallery
