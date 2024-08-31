#include "speechkit_api_builder.h"

#include <alice/library/json/json.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>
#include <alice/megamind/protos/common/location.pb.h>

#include <library/cpp/timezone_conversion/civil.h>

#include <util/datetime/base.h>
#include <util/string/cast.h>
#include <util/string/vector.h>

namespace NAlice {

const TString TSpeechKitApiRequestBuilder::PredefinedSession =
    "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";

TSpeechKitApiRequestBuilder::TSpeechKitApiRequestBuilder(TStringBuf json)
    : TSpeechKitApiRequestBuilder(JsonToProto<TSpeechKitRequestProto>(JsonFromString(json)))
{
}

TSpeechKitApiRequestBuilder::TSpeechKitApiRequestBuilder()
    : TSpeechKitApiRequestBuilder(TSpeechKitRequestProto{})
{
    UpdateHeader(/* seqNumber= */0);
    UpdateUserTime();
    UpdateLang();
    SetClientIp();
    UpdateServerTime();
}

TSpeechKitApiRequestBuilder::TSpeechKitApiRequestBuilder(TSpeechKitRequestProto proto)
    : Proto_{std::move(proto)}
{
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::SetTextInput(const TString& text) {
    auto& event = *Proto_.MutableRequest()->MutableEvent();
    event.Clear();
    event.SetText(text);
    event.SetType(NAlice::EEventType::text_input);
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::SetVoiceInput(const TString& text) {
    auto& event = *Proto_.MutableRequest()->MutableEvent();
    event.Clear();
    event.SetEndOfUtterance(true);
    event.SetType(NAlice::EEventType::voice_input);
    auto& asrResult = *event.AddAsrResult();
    asrResult.SetUtterance(text);
    asrResult.SetNormalized(text);
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::SetAsrResult(const TVector<TVector<TString>>& asrHypothesesWords) {
    auto& event = *Proto_.MutableRequest()->MutableEvent();
    event.Clear();
    event.SetEndOfUtterance(true);
    event.SetType(NAlice::EEventType::voice_input);
    for (const auto& asrHypothesisWords : asrHypothesesWords) {
        auto& asrResult = *event.AddAsrResult();
        const auto utterance = JoinStrings(asrHypothesisWords, " ");
        asrResult.SetUtterance(utterance);
        asrResult.SetNormalized(utterance);
        for (const auto& word : asrHypothesisWords) {
            asrResult.AddWords()->SetValue(word);
        }
    }
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::ClearExperiments() {
    Proto_.MutableRequest()->ClearExperiments();
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::EnableExpFlag(const TString& flag) {
    static const TString enable = "1";
    return SetValueExpFlag(flag, enable);
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::DisableExpFlag(const TString& flag) {
    static const TString disable = "0";
    return SetValueExpFlag(flag, disable);
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::RemoveExpFlag(const TString& flag) {
    auto& storage = *Proto_.MutableRequest()->MutableExperiments()->MutableStorage();
    storage.erase(flag);
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::SetValueExpFlag(const TString& flag, const TString& value) {
    auto& storage = *Proto_.MutableRequest()->MutableExperiments()->MutableStorage();
    auto it = storage.find(flag);
    if (it != storage.end()) {
        it->second.SetString(value);
    } else {
        storage[flag].SetString(value);
    }

    return *this;
}


TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::SetOAuthToken(const TString& token) {
    Proto_.MutableRequest()->MutableAdditionalOptions()->SetOAuthToken(token);
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::UpdateUserTime(TInstant time, const TString& tzName) {
    const auto tz = NDatetime::GetTimeZone(tzName);
    TString clientTime = NDatetime::Format("%Y%m%dT%H%M%S", NDatetime::Convert(time, tz), tz);

    auto& app = *Proto_.MutableApplication();
    app.SetEpoch(ToString(time.TimeT()));
    app.SetClientTime(std::move(clientTime));
    app.SetTimezone(tzName);

    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::UpdateHeader(ui32 seqNum) {
    auto& header = *Proto_.MutableHeader();
    header.SetRequestId("test_request_id");
    header.SetSequenceNumber(seqNum);
    if (seqNum > 0) {
        header.SetPrevReqId("prev_test_request_id");
    }

    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::UpdateLang(const TString& lang) {
    Proto_.MutableApplication()->SetLang(lang);
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::SetLocation(TGeoPos position, double accuracy, double recency) {
    auto& location = *Proto_.MutableRequest()->MutableLocation();
    location.Clear();
    location.SetLat(position.Lat);
    location.SetLon(position.Lon);
    location.SetRecency(recency);
    location.SetAccuracy(accuracy);

    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::SetClientIp(const TString& ip) {
    Proto_.MutableRequest()->MutableAdditionalOptions()->MutableBassOptions()->SetClientIP(ip);
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::ClearClientIp() {
    Proto_.MutableRequest()->MutableAdditionalOptions()->MutableBassOptions()->ClearClientIP();
    return *this;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::SetPredefinedClient(EClient client) {
    // XXX Rewrite it!

    auto& app = *Proto_.MutableApplication();

    switch (client) {
        case EClient::SearchApp:
            app.SetAppId("ru.yandex.searchplugin");
            app.SetAppVersion("9.20");
            break;
        case EClient::Quasar:
            app.SetAppId("ru.yandex.quasar");
            app.SetAppVersion("1.0");
            break;
    }

    return *this;
}

NJson::TJsonValue TSpeechKitApiRequestBuilder::BuildJson() const {
    return JsonFromProto(Proto_);
}

TSpeechKitRequestProto TSpeechKitApiRequestBuilder::BuildProto() const {
    return Proto_;
}

TSpeechKitRequestProto& TSpeechKitApiRequestBuilder::RawProto() {
    return Proto_;
}

TSpeechKitApiRequestBuilder& TSpeechKitApiRequestBuilder::UpdateServerTime(const TInstant time) {
    Proto_.MutableRequest()->MutableAdditionalOptions()->SetServerTimeMs(time.MilliSeconds());
    return *this;
}

} // NAlice
