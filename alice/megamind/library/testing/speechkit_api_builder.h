#pragma once

#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/datetime/base.h>

namespace NAlice {

/** XXX A very initial and superficial implementation
 *  of speechkit requests builder.
 *  API will be modified.
 */
class TSpeechKitApiRequestBuilder {
public:
    enum class EClient {
        SearchApp,
        Quasar,
    };

    struct TGeoPos {
        double Lat;
        double Lon;
    };
    static inline constexpr TGeoPos MoscowLocation = {55.59355164, 37.60249329};

    static const TString PredefinedSession;

public:
    /** Construct an empty protobuf TSpeechKitRequestProto and
     *  fill in some predefined defaults.
     */
    TSpeechKitApiRequestBuilder();

    /** The following constructors just parse json and do not
     *  apply predefined defaults.
     */
    explicit TSpeechKitApiRequestBuilder(TStringBuf json);
    explicit TSpeechKitApiRequestBuilder(const NJson::TJsonValue& json);
    explicit TSpeechKitApiRequestBuilder(TSpeechKitRequestProto proto);

    NJson::TJsonValue BuildJson() const;
    TSpeechKitRequestProto BuildProto() const;

    // Direct access to the protobuf.
    const TSpeechKitRequestProto& RawProto() const;
    TSpeechKitRequestProto& RawProto();

    // Events.
    TSpeechKitApiRequestBuilder& SetTextInput(const TString& text);
    TSpeechKitApiRequestBuilder& SetVoiceInput(const TString& text);
    TSpeechKitApiRequestBuilder& SetAsrResult(const TVector<TVector<TString>>& asrHypothesesWords);

    // Experiments.
    TSpeechKitApiRequestBuilder& ClearExperiments();
    TSpeechKitApiRequestBuilder& EnableExpFlag(const TString& flag);
    TSpeechKitApiRequestBuilder& DisableExpFlag(const TString& flag);
    TSpeechKitApiRequestBuilder& RemoveExpFlag(const TString& flag);
    // XXX Use it with care because expflags protocol is not compeletely specified!
    TSpeechKitApiRequestBuilder& SetValueExpFlag(const TString& flag, const TString& value);

    // Aux info.
    TSpeechKitApiRequestBuilder& SetOAuthToken(const TString& token);
    // Can throw NDatetime::TInvalidTimezone.
    TSpeechKitApiRequestBuilder& UpdateUserTime(TInstant time = TInstant::Now(), const TString& tz = "Europe/Moscow");
    TSpeechKitApiRequestBuilder& UpdateHeader(ui32 seqNumber);
    TSpeechKitApiRequestBuilder& UpdateLang(const TString& lang = "ru-RU");
    TSpeechKitApiRequestBuilder& SetLocation(TGeoPos position = MoscowLocation, double accuracy = 1.0, double recency = 1.0);

    TSpeechKitApiRequestBuilder& SetClientIp(const TString& ip = "127.0.0.1");
    TSpeechKitApiRequestBuilder& ClearClientIp();

    TSpeechKitApiRequestBuilder& UpdateServerTime(TInstant time = TInstant::Now());

    // XXX Use with care, will be rewritten!
    TSpeechKitApiRequestBuilder& SetPredefinedClient(EClient client);

private:
    TSpeechKitRequestProto Proto_;
};

} // NAlice
