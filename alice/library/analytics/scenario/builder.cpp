#include "builder.h"

#include <alice/megamind/protos/property/property.pb.h>
#include <alice/megamind/protos/property/iot_profile.pb.h>

#include <alice/library/json/json.h>

namespace NAlice::NScenarios {

namespace {

class TRequestSourceBuilder final : public IAnalyticsInfoBuilder::IRequestSourceBuilder {
public:
    using TProto = TAnalyticsInfo::TEvent::TRequestSourceEvent;

    TRequestSourceBuilder(IAnalyticsInfoBuilder& analyticsInfoBuilder, TProto& proto);

    // IRequestSourceBuilder overrides:
    TRequestSourceBuilder& AddHeader(const TString& header, const TString& value) override;
    TRequestSourceBuilder& AddCgiParam(const TString& key, const TString& value) override;

    TRequestSourceBuilder& SetResponseCode(ui32 code, bool success) override;
    TRequestSourceBuilder& SetResponseBody(const TString& body) override;

    IAnalyticsInfoBuilder& Build() const override;

private:
    IAnalyticsInfoBuilder& Parent;
    TProto& Proto;
};

class TSelectedWebDocumentBuilder final : public IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder {
public:
    using TProto = TAnalyticsInfo::TEvent::TSelectedWebDocumentEvent;

    TSelectedWebDocumentBuilder(IAnalyticsInfoBuilder& analyticsInfoBuilder, TProto& proto);

    // ISelectedWebDocumentBuilder overrides:
    ISelectedWebDocumentBuilder& SetRequestId(const TString& reqId) override;
    ISelectedWebDocumentBuilder& SetSearchRequestId(const TString& searchReqId) override;
    ISelectedWebDocumentBuilder& SetDocumentUrl(const TString& documentUrl) override;
    ISelectedWebDocumentBuilder& SetDocumentPos(ui32 documentPos) override;
    ISelectedWebDocumentBuilder& SetCatalogUrl(const TString& catalogUrl) override;
    ISelectedWebDocumentBuilder& SetAnswerUrl(const TString& answerUrl) override;

    IAnalyticsInfoBuilder& Build() const override;

private:
    IAnalyticsInfoBuilder& Parent;
    TProto& Proto;
};

class TVideoRequestSourceBuilder final : public IAnalyticsInfoBuilder::IVideoRequestSourceBuilder {
public:
    using TProto = TAnalyticsInfo::TEvent::TVideoRequestSourceEvent;

    TVideoRequestSourceBuilder(IAnalyticsInfoBuilder& analyticsInfoBuilder, TProto& proto);

    // IRequestSourceBuilder overrides:
    TVideoRequestSourceBuilder& AddHeader(const TString& header, const TString& value) override;
    TVideoRequestSourceBuilder& AddCgiParam(const TString& key, const TString& value) override;

    TVideoRequestSourceBuilder& SetResponseCode(ui32 code, bool success) override;
    TVideoRequestSourceBuilder& SetResponseBody(const TString& body) override;
    TVideoRequestSourceBuilder& SetRequestUrl(const TString& url) override;
    TVideoRequestSourceBuilder& SetRequestText(const TString& text) override;

    IAnalyticsInfoBuilder& Build() const override;

private:
    IAnalyticsInfoBuilder& Parent;
    TProto& Proto;
};

class TProfileBuilder final : public IUserInfoBuilder::IProfileBuilder {
public:
    using TProto = TProperty::TProfile;

    TProfileBuilder(IUserInfoBuilder& userInfoBuilder, TProto& proto);

    // IProfileBuilder overrides:
    TProfileBuilder& AddParams(const TString& key, const TString& value, const TString& description) override;

    IUserInfoBuilder& Build() const override;

private:
    IUserInfoBuilder& Parent;
    TProto& Proto;
};

// TRequestSourceBuilder -------------------------------------------------------
TRequestSourceBuilder::TRequestSourceBuilder(IAnalyticsInfoBuilder& builder, TProto& proto)
    : Parent(builder)
    , Proto(proto) {
}

TRequestSourceBuilder& TRequestSourceBuilder::AddHeader(const TString& header, const TString& value) {
    (*Proto.MutableHeaders())[header] = value;
    return *this;
}

TRequestSourceBuilder& TRequestSourceBuilder::AddCgiParam(const TString& key, const TString& value) {
    (*Proto.MutableCGI())[key] = value;
    return *this;
}

TRequestSourceBuilder& TRequestSourceBuilder::SetResponseCode(ui32 code, bool success) {
    Proto.SetResponseCode(code);
    Proto.SetResponseSuccess(success);
    return *this;
}

TRequestSourceBuilder& TRequestSourceBuilder::SetResponseBody(const TString& body) {
    Proto.SetResponseBody(body);
    return *this;
}

IAnalyticsInfoBuilder& TRequestSourceBuilder::Build() const {
    return Parent;
}

// TSelectedWebDocumentBuilder -------------------------------------------------
TSelectedWebDocumentBuilder::TSelectedWebDocumentBuilder(IAnalyticsInfoBuilder& builder, TProto& proto)
    : Parent(builder)
    , Proto(proto) {
}

IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder&
TSelectedWebDocumentBuilder::SetRequestId(const TString& reqId) {
    Proto.SetRequestId(reqId);
    return *this;
}

IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder&
TSelectedWebDocumentBuilder::SetSearchRequestId(const TString& searchReqId) {
    Proto.SetSearchRequestId(searchReqId);
    return *this;
}

IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder&
TSelectedWebDocumentBuilder::SetDocumentUrl(const TString& documentUrl) {
    Proto.SetDocumentUrl(documentUrl);
    return *this;
}

IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder&
TSelectedWebDocumentBuilder::SetDocumentPos(ui32 documentPos) {
    Proto.SetDocumentPos(documentPos);
    return *this;
}

IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder&
TSelectedWebDocumentBuilder::SetCatalogUrl(const TString& catalogUrl) {
    Proto.SetCatalogUrl(catalogUrl);
    return *this;
}

IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder&
TSelectedWebDocumentBuilder::SetAnswerUrl(const TString& answerUrl) {
    Proto.SetAnswerUrl(answerUrl);
    return *this;
}

IAnalyticsInfoBuilder& TSelectedWebDocumentBuilder::Build() const {
    return Parent;
}

// TVideoRequestSourceBuilder -------------------------------------------------------
TVideoRequestSourceBuilder::TVideoRequestSourceBuilder(IAnalyticsInfoBuilder& builder, TProto& proto)
    : Parent(builder)
    , Proto(proto) {
}

TVideoRequestSourceBuilder& TVideoRequestSourceBuilder::AddHeader(const TString& header, const TString& value) {
    (*Proto.MutableHeaders())[header] = value;
    return *this;
}

TVideoRequestSourceBuilder& TVideoRequestSourceBuilder::AddCgiParam(const TString& key, const TString& value) {
    (*Proto.MutableCGI())[key] = value;
    return *this;
}

TVideoRequestSourceBuilder& TVideoRequestSourceBuilder::SetResponseCode(ui32 code, bool success) {
    Proto.SetResponseCode(code);
    Proto.SetResponseSuccess(success);
    return *this;
}

TVideoRequestSourceBuilder& TVideoRequestSourceBuilder::SetResponseBody(const TString& body) {
    Proto.SetResponseBody(body);
    return *this;
}

TVideoRequestSourceBuilder& TVideoRequestSourceBuilder::SetRequestUrl(const TString& url) {
    Proto.SetRequestUrl(url);
    return *this;
}

TVideoRequestSourceBuilder& TVideoRequestSourceBuilder::SetRequestText(const TString& text) {
    Proto.SetRequestText(text);
    return *this;
}

IAnalyticsInfoBuilder& TVideoRequestSourceBuilder::Build() const {
    return Parent;
}

// TProfileBuilder -------------------------------------------------------------
TProfileBuilder::TProfileBuilder(IUserInfoBuilder& builder, TProto& proto)
    : Parent(builder)
    , Proto(proto) {
}

TProfileBuilder& TProfileBuilder::AddParams(const TString& key, const TString& value, const TString& description) {
    TProperty::TProfile::TParamsValue property;
    property.SetValue(value);
    property.SetHumanReadable(description);

    (*Proto.MutableParams())[key] = property;
    return *this;
}

IUserInfoBuilder& TProfileBuilder::Build() const {
    return Parent;
}

} // namespace

// TAnalyticsInfoBuilder -------------------------------------------------------
TAnalyticsInfoBuilder::TAnalyticsInfoBuilder(TAnalyticsInfo& analyticsInfo)
    : AnalyticsInfo{analyticsInfo} {
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::SetIntentName(const TString& intentName) {
    AnalyticsInfo.SetIntent(intentName);
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::SetProductScenarioName(const TString& productScenarioName) {
    AnalyticsInfo.SetProductScenarioName(productScenarioName);
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddActionsFromProto(const TAnalyticsInfo& analyticsInfo) {
    for (const auto& action : analyticsInfo.GetActions()) {
        *AnalyticsInfo.MutableActions()->Add() = action;
    }
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddActions(const TVector<TAnalyticsInfo::TAction>& actions) {
    for (const auto& action : actions) {
        *AnalyticsInfo.MutableActions()->Add() = action;
    }
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddAction(const TString& id, const TString& name,
                                                        const TString& description) {
    auto* object = AnalyticsInfo.AddActions();
    object->SetId(id);
    object->SetName(name);
    object->SetHumanReadable(description);
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddAction(const TAnalyticsInfo::TAction& action) {
    *AnalyticsInfo.MutableActions()->Add() = action;
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddObjectsFromProto(const TAnalyticsInfo& analyticsInfo) {
    for (const auto& object : analyticsInfo.GetObjects()) {
        *AnalyticsInfo.MutableObjects()->Add() = object;
    }
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddObject(const TString& id, const TString& name,
                                                        const TString& description) {
    auto* object = AnalyticsInfo.AddObjects();
    object->SetId(id);
    object->SetName(name);
    object->SetHumanReadable(description);
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddObject(const TAnalyticsInfo::TObject& object) {
    *AnalyticsInfo.MutableObjects()->Add() = object;
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddEventsFromProto(const TAnalyticsInfo& analyticsInfo) {
    for (const auto& event : analyticsInfo.GetEvents()) {
        *AnalyticsInfo.MutableEvents()->Add() = event;
    }
    return *this;
}

THolder<IAnalyticsInfoBuilder::IRequestSourceBuilder>
TAnalyticsInfoBuilder::AddRequestSourceEvent(const TInstant& instant, const TString& source) {
    auto* event = AnalyticsInfo.AddEvents();
    event->SetTimestamp(instant.MicroSeconds());

    auto* requestSourceEvent = event->MutableRequestSourceEvent();
    requestSourceEvent->SetSource(source);
    return MakeHolder<TRequestSourceBuilder>(*this, *requestSourceEvent);
}

IAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddSelectedSourceEvent(const TInstant& instant, const TString& source) {
    auto* event = AnalyticsInfo.AddEvents();
    event->SetTimestamp(instant.MicroSeconds());

    auto* selectedSourceEvent = event->MutableSelectedSourceEvent();
    selectedSourceEvent->SetSource(source);
    return *this;
}

THolder<IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder>
TAnalyticsInfoBuilder::AddSelectedWebDocumentEvent(const TInstant& instant, const TString& searchType) {
    auto* event = AnalyticsInfo.AddEvents();
    event->SetTimestamp(instant.MicroSeconds());

    auto* selectedWebDocumentEvent = event->MutableSelectedWebDocumentEvent();
    selectedWebDocumentEvent->SetSearchType(searchType);
    return MakeHolder<TSelectedWebDocumentBuilder>(*this, *selectedWebDocumentEvent);
}

IAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddAnalyticsInfoMusicEvent(
    const TInstant instant,
    const TMusicEventAnswerType answerType,
    const TString& id,
    const TString& uri)
{
    auto* event = AnalyticsInfo.AddEvents();
    event->SetTimestamp(instant.MicroSeconds());

    auto* musicEvent = event->MutableMusicEvent();
    musicEvent->SetAnswerType(answerType);

    if (id) {
        musicEvent->SetId(id);
    }

    if (uri) {
        musicEvent->SetUri(uri);
    }

    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddAnalyticsInfoMusicMonitoringEvent(const TInstant& instant,
    bool batchOfTracksRequested, bool cacheHit) {
    auto* event = AnalyticsInfo.AddEvents();
    event->SetTimestamp(instant.MicroSeconds());

    auto* musicMonitoringEvent = event->MutableMusicMonitoringEvent();
    musicMonitoringEvent->SetBatchOfTracksRequested(batchOfTracksRequested);
    musicMonitoringEvent->SetCacheHit(cacheHit);

    return *this;
}

THolder<IAnalyticsInfoBuilder::IVideoRequestSourceBuilder>
TAnalyticsInfoBuilder::AddVideoRequestSourceEvent(const TInstant& instant, const TString& source) {
    auto* event = AnalyticsInfo.AddEvents();
    event->SetTimestamp(instant.MicroSeconds());

    auto* videoRequestSourceEvent = event->MutableVideoRequestSourceEvent();
    videoRequestSourceEvent->SetSource(source);
    return MakeHolder<TVideoRequestSourceBuilder>(*this, *videoRequestSourceEvent);
}


TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddTunnellerRawResponse(const TString& response) {
    AnalyticsInfo.AddTunnellerRawResponses(response);
    return *this;
}

IAnalyticsInfoBuilder& TAnalyticsInfoBuilder::SetStageStartTime(const TString& stage, TInstant startTime) {
    (*AnalyticsInfo.MutableScenarioTimings()->MutableTimings())[stage].SetStartTimestamp(startTime.MicroSeconds());
    return *this;
}

IAnalyticsInfoBuilder& TAnalyticsInfoBuilder::AddSourceResponseDuration(const TString& stage, const TString& source,
                                                                        TDuration duration) {
    (*(*AnalyticsInfo.MutableScenarioTimings()->MutableTimings())[stage].MutableSourceResponseDurations())[source] =
        duration.MicroSeconds();
    return *this;
}

// TUserInfoBuilder ------------------------------------------------------------
TUserInfoBuilder::TUserInfoBuilder(TUserInfo& userInfo)
    : UserInfo{userInfo} {
}

THolder<IUserInfoBuilder::IProfileBuilder> TUserInfoBuilder::AddProfile(const TString& id, const TString& name,
                                                                        const TString& description) {
    auto* property = UserInfo.AddProperties();
    property->SetId(id);
    property->SetName(name);
    property->SetHumanReadable(description);
    return MakeHolder<TProfileBuilder>(*this, *property->MutableProfile());
}

TUserInfoBuilder& TUserInfoBuilder::AddIotProfile(const NJson::TJsonValue& iotProfileJson) {
    TIotProfile iotProfile;
    JsonToProto(iotProfileJson, iotProfile, /* validateUtf8= */ true, /* ignoreUnknownFields= */ true);

    auto* object = UserInfo.AddProperties();
    object->SetHumanReadable("Smart home configuration");
    *object->MutableIotProfile() = std::move(iotProfile);

    return *this;
}

} // namespace NAlice::NScenarios
