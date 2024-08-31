#pragma once

#include <alice/bass/libs/push_notification/scheme/scheme.sc.h>

#include <alice/bass/libs/client/client_info.h>

#include <alice/bass/util/error.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/generic/ptr.h>

namespace NBASS::NPushNotification {

struct THandler {
public:
    using TCallbackData = NBassPushNotification::TCallbackDataConst<TSchemeTraits>;

    THandler(const TCallbackData& callbackData)
        : CallbackData(callbackData)
        , ClientInfo(TString{*callbackData->ClientId()})
    {
    }

    TString GetUUId() const {
        return TString{*CallbackData->UUId()};
    }

    TString GetDId() const {
        return TString{*CallbackData->DId()};
    }

    TString GetUId() const {
        return TString{*CallbackData->UId()};
    }

    const TClientInfo& GetClientInfo() const {
        return ClientInfo;
    }

    TStringBuf GetEvent() const {
        return Event;
    }

    TStringBuf GetQuasarEvent() const {
        return QuasarEvent;
    }

    TStringBuf GetBody() const {
        return Body;
    }

    TStringBuf GetTitle() const {
        return Title;
    }

    TStringBuf GetUrl() const {
        return Url;
    }

    TStringBuf GetQuasarPayload() const {
        return QuasarPayload;
    }

    TStringBuf GetTag() const {
        return Tag;
    }

    TStringBuf GetThrottlePolicy() const {
        return ThrottlePolicy;
    }

    int GetTTL() const {
        return TTL;
    }

    const TVector<TString>& GetPushes() const {
        return Pushes;
    }

    void SetInfo(TStringBuf event, TStringBuf quasarEvent, TStringBuf title, TStringBuf body, TStringBuf url, TStringBuf tag, int ttl, TStringBuf quasarPayload = {}, TStringBuf throttlePolicy = "bass-default-push") {
        Event = TString{event};
        QuasarEvent = TString{quasarEvent};
        Title = TString{title};
        Body = TString{body};
        Url = TString{url};
        Tag = TString{tag};
        TTL = ttl;
        QuasarPayload = TString{quasarPayload};
        ThrottlePolicy = throttlePolicy;
    }

    // Adds notification to the device, from which the payload was made
    void AddSelf() {
        Pushes.push_back(ClientInfo.Name);
    }

    // Adds notification to user's device with specified client_id
    // Client_ids can be found here: https://wiki.yandex-team.ru/sup/apps/
    void AddCustom(const TString& clientId) {
        if (ClientInfo.Name != clientId && Find(Pushes, clientId) == Pushes.end()) {
            Pushes.push_back(clientId);
        }
    }

private:
    TCallbackData CallbackData;

    TClientInfo ClientInfo;

    TString Event;
    TString QuasarEvent;
    TString Title;
    TString Body;
    TString Url;
    TString QuasarPayload;
    TString Tag;
    TString ThrottlePolicy;
    int TTL;

    TVector<TString> Pushes;
};


class IHandlerGenerator {
public:
    using TApiScheme = NBassPushNotification::TApiRequest<TSchemeTraits>;
    using TApiSchemeHolder = TSchemeHolder<TApiScheme>;

    virtual ~IHandlerGenerator() = default;

    virtual TResultValue Generate(THandler& handler, TApiSchemeHolder scheme) = 0;
};

} // namespace NBASS::NPushNotification
