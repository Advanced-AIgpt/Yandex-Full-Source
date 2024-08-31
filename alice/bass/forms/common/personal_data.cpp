#include "personal_data.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/forms/navigator/bookmarks_matcher.h>
#include <alice/bass/forms/navigator/navigator_intent.h>
#include <alice/bass/forms/navigator/user_bookmarks.h>
#include <alice/bass/forms/special_location.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/tvm2/tvm2_ticket_cache.h>

#include <alice/library/blackbox/blackbox_http.h>
#include <alice/library/network/headers.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/stream/output.h>
#include <util/string/builder.h>
#include <util/string/subst.h>
#include <util/system/yassert.h>

#include <memory>

namespace NBASS {

namespace {

constexpr TStringBuf ADDRESSES_URL_SUFFIX = "/v2/personality/profile/addresses";
constexpr TStringBuf BROADCAST_URL_SUFFIX = "/v1/batch/request";
constexpr TStringBuf KV_URL_SUFFIX = "/v1/personality/profile/alisa/kv";

constexpr TStringBuf CONTENT_TYPE_JSON = "application/json; charset=utf-8";

class TSaveAddressNavigatorIntent : public INavigatorIntent {
public:
    TSaveAddressNavigatorIntent(TContext& ctx, TStringBuf type, TGeoPosition geo)
        : INavigatorIntent(ctx, TStringBuf("set_place") /* scheme */)
        , Type(type)
        , Geo(geo) {
    }

private:
    TResultValue SetupSchemeAndParams() override {
        Params.InsertUnescaped(TStringBuf("type"), Type);
        Params.InsertUnescaped(TStringBuf("lat"), ToString(Geo.Lat));
        Params.InsertUnescaped(TStringBuf("lon"), ToString(Geo.Lon));

        LOG(DEBUG) << "Trying to save <" << Type << "> user address in Yandex.Navigator app" << Endl;
        return TResultValue();
    }

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorSetPlaceDirective>();
    }

private:
    TString Type;
    TGeoPosition Geo;
};

class TMessageBuilder {
public:
    explicit TMessageBuilder(const std::function<void(TStringBuilder&)>& fn)
        : Fn(fn) {
    }

    TMessageBuilder& Append(TStringBuf msg) {
        Aux.push_back(msg);
        return *this;
    }

    TMessageBuilder& Log() {
        TStringBuilder msg;
        Fn(msg);
        for (const auto& aux : Aux)
            msg << aux;
        LOG(ERR) << msg << Endl;
        return *this;
    }

private:
    std::function<void(TStringBuilder&)> Fn;
    TVector<TStringBuf> Aux;
};

[[nodiscard]] bool GetTVM2ServiceTicket(IGlobalContext& ctx, TStringBuf serviceId, TString& ticket) {
    auto* ticketCache = ctx.TVM2TicketCache();

    if (!ticketCache) {
        Y_STATS_INC_COUNTER("GetTVM2ServiceTicket_error_CacheUnavailable");
        LOG(ERR) << "TVM 2.0 ticket cache unavailable" << Endl;
        return false;
    }

    if (ctx.IsTVM2Expired()) {
        Y_STATS_INC_COUNTER("GetTVM2ServiceTicket_error_TicketsExpired");
        LOG(ERR) << "TVM 2.0 service tickets expired" << Endl;
        return false;
    }

    if (const auto t = ticketCache->GetTicket(serviceId))
        ticket = *t;
    else
        ticket.clear();

    if (ticket.empty()) {
        LOG(ERR) << "Failed to read TVM 2 service ticket for id " << serviceId << " from cache" << Endl;
        Y_STATS_INC_COUNTER("GetTVM2ServiceTicket_error_EmptyTicketInCache");
        return false;
    }

    Y_STATS_INC_COUNTER("GetTVM2ServiceTicket_success");
    return true;
}

NAlice::TBlackBoxHttpFetcher& GetOrCreateBlackBoxRequest(TContext& ctx, bool isTest = false) {
    if (auto& fetcher = ctx.GetBlackBoxRequest()) {
        return *fetcher;
    }

    ctx.SetBlackBoxRequest(NAlice::TBlackBoxHttpFetcher{});
    auto& fetcher = *ctx.GetBlackBoxRequest();

    const auto& sourcesConfig = ctx.GetConfig().Vins();

    TString ticket;
    auto blackBoxSource = isTest ? sourcesConfig.BlackBoxTest() : sourcesConfig.BlackBox();
    if (!GetTVM2ServiceTicket(ctx.GlobalCtx(), blackBoxSource.Tvm2ClientId(), ticket) || ticket.Empty()) {
        return fetcher;
    }

    auto request = ctx.GetSources().BlackBox(isTest).Request();
    request->AddHeader(NAlice::NNetwork::HEADER_X_YA_SERVICE_TICKET, ticket);
    fetcher.StartRequest(*request, *ctx.Meta().ClientIP(), ctx.UserAuthorizationHeader());

    return fetcher;
}

template <typename TValue>
struct TBlackBoxValueExtractor {
    explicit TBlackBoxValueExtractor(TValue& value)
        : Value(value)
    {
    }

    bool operator()(const TValue& value) {
        Value = value;
        return true;
    }

    bool operator()(const NAlice::TBlackBoxError& error) {
        LOG(ERR) << "Failed to request BlackBox: " << error.Message() << Endl;
        return false;
    }

    TValue& Value;
};

template <typename TValue>
TBlackBoxValueExtractor<TValue> MakeBlackBoxValueExtractor(TValue& value) {
    return TBlackBoxValueExtractor<TValue>(value);
}

void AddUniproxyPutAction(TContext& ctx, TStringBuf key, const NSc::TValue& value) {
    NSc::TValue data;
    data["key"].SetString(key);
    data["method"].SetString(TStringBuf("PUT"));
    data["value"] = value;
    data["listening_is_possible"].SetBool(true);
    ctx.AddUniProxyAction(UNIPROXY_ACTION_UPDATE_DATASYNC, data);
}

void AddUniproxyDeleteAction(TContext& ctx, TStringBuf key) {
    NSc::TValue data;
    data["key"].SetString(key);
    data["method"].SetString(TStringBuf("DELETE"));
    ctx.AddUniProxyAction(UNIPROXY_ACTION_UPDATE_DATASYNC, data);
}

struct TKeyToStringConverter {
    TString operator()(TPersonalDataHelper::EUserSpecificKey key) {
        return ToString(key);
    }

    TString operator()(TPersonalDataHelper::EUserDeviceSpecificKey key) {
        return ToString(key);
    }
};

const NSc::TValue& GetPersonalData(const TContext& ctx) {
    return ctx.Meta().HasPersonalData() ? *ctx.Meta().PersonalData().GetRawValue() : NSc::Null();
}

struct TDataSyncKeyBuilder {
    const TContext& Ctx;

    TString operator()(TPersonalDataHelper::EUserSpecificKey key) {
        return ToString(key);
    }

    TString operator()(TPersonalDataHelper::EUserDeviceSpecificKey key) {
        return TStringBuilder() << Ctx.GetDeviceModel() << "_" << Ctx.GetDeviceId() << "_" << ToString(key);
    }
};

TString ToDataSyncKey(const TContext& ctx, TPersonalDataHelper::EKey key) {
    return TStringBuilder() << KV_URL_SUFFIX << '/' << std::visit(TDataSyncKeyBuilder{ctx}, key);
}

} // namespace

TPersonalDataHelper::TPersonalDataHelper(TContext& context, bool isTestBlackBox)
    : Ctx(context)
    , IsTestBlackBox(isTestBlackBox)
    , PersonalData(GetPersonalData(context))
    , UseUniproxyDatasyncProtocol(!context.HasExpFlag(EXPERIMENTAL_FLAG_NO_DATASYNC_UNIPROXY))
{
}

bool TPersonalDataHelper::GetUid(TString& uid) const {
    // For mocking in tests
    if (Ctx.Meta().HasUID()) {
        uid = ToString(Ctx.Meta().UID());
        return true;
    }

    // If we have BlackBox datasource, we can use it.
    if (const auto* bbSource = Ctx.DataSources().FindPtr(BLACK_BOX_TYPE)) {
        const TStringBuf bbSourceUid = (*bbSource)["user_info"]["uid"].GetString();
        if (!bbSourceUid.empty()) {
            LOG(INFO) << "Got uid from data sources" << Endl;
            uid = bbSourceUid;
            return true;
        }
        LOG(ERR) << "UID is missing in blackbox." << Endl;
    }

    auto& blackBox = GetOrCreateBlackBoxRequest(Ctx, IsTestBlackBox);
    return Visit(MakeBlackBoxValueExtractor(uid), blackBox.GetUid());
}

TMaybe<TString> TPersonalDataHelper::GetUid() const {
    if (TString uid; GetUid(uid)) {
        return uid;
    }
    return Nothing();
}

bool TPersonalDataHelper::GetUserInfo(TUserInfo& info) const {
    NAlice::TBlackBoxFullUserInfoProto proto;
    auto& blackBox = GetOrCreateBlackBoxRequest(Ctx, IsTestBlackBox);
    bool result = Visit(MakeBlackBoxValueExtractor(proto), blackBox.GetFullInfo());
    if (result) {
        info = std::move(*proto.MutableUserInfo());
    }
    return result;
}

bool TPersonalDataHelper::GetTVM2UserTicket(TString& ticket) const {
    if (const auto& userTicket = Ctx.UserTicket()) {
        LOG(INFO) << "User ticket is present, skipping request" << Endl;
        ticket = *userTicket;
        return true;
    }
    auto& blackBox = GetOrCreateBlackBoxRequest(Ctx, IsTestBlackBox);
    return Visit(MakeBlackBoxValueExtractor(ticket), blackBox.GetTVM2UserTicket());
}

bool TPersonalDataHelper::GetTVM2ServiceTicket(TStringBuf serviceId, TString& ticket) const {
    return ::NBASS::GetTVM2ServiceTicket(Ctx.GlobalCtx(), serviceId, ticket);
}

TSavedAddress TPersonalDataHelper::GetDataSyncUserAddress(TStringBuf addressId) const {
    TString addressUrl = TStringBuilder() << ADDRESSES_URL_SUFFIX << '/' << addressId;
    if (UseUniproxyDatasyncProtocol) {
        const auto& value = PersonalData.Get(addressUrl);
        return value.IsNull() ? TSavedAddress() : TSavedAddress(value);
    }

    auto request = CreateDataSyncRequest(addressUrl);

    AddTVM2AuthorizationHeaders(*request.Get(), Ctx.IsAuthorizedUser());

    auto resp = request->Fetch()->Wait();

    if (!resp || resp->IsError()) {
        LOG(ERR) << "Failed to obtain <" << addressId << "> user address"
                 << " (" << (Ctx.IsAuthorizedUser() ? "authorized" : "unauthorized")
                 << " user) : " << (resp ? resp->GetErrorText() : "no reply from server") << Endl;
        return TSavedAddress();
    }

    return TSavedAddress(NSc::TValue::FromJson(resp->Data));
}

TResultValue TPersonalDataHelper::SaveDataSyncUserAddress(TStringBuf addressId, const TSavedAddress& address) {
    TString addressUrl = TStringBuilder() << ADDRESSES_URL_SUFFIX << '/' << addressId;
    const auto& value = address.Value();
    if (UseUniproxyDatasyncProtocol) {
        AddUniproxyPutAction(Ctx, addressUrl, value);
        return TResultValue();
    }

    auto request = CreateDataSyncRequest(addressUrl);
    AddTVM2AuthorizationHeaders(*request.Get(), Ctx.IsAuthorizedUser());
    auto resp = request->SetMethod("PUT").SetBody(value.ToJson()).SetContentType(CONTENT_TYPE_JSON).Fetch()->Wait();

    if (!resp || resp->IsError()) {
        LOG(ERR) << "Failed to update < " << addressId << "> user address"
                 << " (" << (Ctx.IsAuthorizedUser() ? "authorized" : "unauthorized")
                 << " user) : " << (resp ? resp->GetErrorText() : "no reply from server") << Endl;
        return TError(TError::EType::SYSTEM, "Failed to save address");
    }

    return TResultValue();
}

TResultValue TPersonalDataHelper::UpdateDataSyncDeviceGeoPoint(TStringBuf uid, TStringBuf geoPointName) {
    NSc::TValue currentValue;
    currentValue["location"].SetString(geoPointName);

    if (auto err = SaveDataSyncJsonValue(uid, EUserDeviceSpecificKey::Location, currentValue)) {
        return err;
    }
    return TResultValue();
}

TResultValue TPersonalDataHelper::DeleteDataSyncUserAddress(TStringBuf addressId) const {
    TString addressUrl = TStringBuilder() << ADDRESSES_URL_SUFFIX << '/' << addressId;
    if (UseUniproxyDatasyncProtocol) {
        if (!Ctx.IsAuthorizedUser()) {
            return TError(TError::EType::SYSTEM, "Deleting address for unauthorized user");
        }
        AddUniproxyDeleteAction(Ctx, addressUrl);
        return TResultValue();
    }

    auto request = CreateDataSyncRequest(addressUrl);
    AddTVM2AuthorizationHeaders(*request.Get(), Ctx.IsAuthorizedUser());
    auto resp = request->SetMethod("DELETE").Fetch()->Wait();

    if (!resp || resp->IsError()) {
        LOG(ERR) << "Failed to delete <" << addressId << "> user address"
                 << " (" << (Ctx.IsAuthorizedUser() ? "authorized" : "unauthorized")
                 << " user) : " << (resp ? resp->GetErrorText() : "no reply from server") << Endl;
        return TError(TError::EType::SYSTEM, "Failed to delete address");
    }

    return TResultValue();
}

TResultValue TPersonalDataHelper::SaveDataSyncBatch(TStringBuf uid, TStringBuf body) {
    if (!uid) {
        LOG(ERR) << "No uid for datasync" << Endl;
        return TError(TError::EType::SYSTEM, "No uid");
    }

    auto request = CreateDataSyncRequest(TStringBuilder() << BROADCAST_URL_SUFFIX);

    AddTVM2AuthorizationHeaders(*request.Get(), uid);

    request->SetMethod("POST");
    request->SetContentType(CONTENT_TYPE_JSON);
    request->SetBody(body);

    auto response = request->Fetch()->Wait();

    return VerifyBatchResponse(response, [&uid](TStringBuilder& message) {
        message << "Failed to save DataSync values for UID: " << uid;
    } /* onError */);
}

TResultValue TPersonalDataHelper::SaveDataSyncKeyValues(TStringBuf uid, const TVector<TKeyValue>& kvs) {
    if (kvs.empty())
        return TResultValue();
    if (UseUniproxyDatasyncProtocol) {
        for (const auto& keyValue : kvs) {
            AddUniproxyPutAction(Ctx, ToDataSyncKey(Ctx, keyValue.Key), NSc::TValue(keyValue.Value));
        }
        return TResultValue();
    }
    return SaveDataSyncBatch(uid, PrepareDataSyncBatchRequestContent(kvs).ToJson());
}

TResultValue TPersonalDataHelper::GetDataSyncJsonValue(TStringBuf uid, EKey key, NSc::TValue &jsonValue) {
    TString value;
    if (!uid) {
        LOG(ERR) << "No uid for datasync" << Endl;
        return TError(TError::EType::SYSTEM, "No uid");
    }
    if (auto err = GetDataSyncKeyValue(uid, key, value)) {
        LOG(ERR) << "Personal data request error: "  << err << Endl;
        return err;
    }
    if (!NSc::TValue::FromJson(jsonValue, value)) {
        LOG(ERR) << "Bad json for uid: "  << uid << "in datasync " << ToDataSyncKey(Ctx, key) << ": " << value << Endl;
        return TError(TError::EType::SYSTEM, "Bad json in datasync");
    }
    return TResultValue();
}

NHttpFetcher::TRequestPtr TPersonalDataHelper::CreateDataSyncRequest(TStringBuf path) const {
    return Ctx.GetSources().PersonalData(path, IsTestBlackBox).Request();
}

TResultValue TPersonalDataHelper::SaveDataSyncJsonValue(TStringBuf uid,  EKey key, const NSc::TValue &jsonValue) {
    TPersonalDataHelper::TKeyValue keyValue{key, jsonValue.ToJson()};
    if (auto err = SaveDataSyncKeyValues(uid, {keyValue})) {
        LOG(ERR) << "Personal data save error: "  << err << Endl;
        return err;
    }
    return TResultValue();
}

NHttpFetcher::THandle::TRef TPersonalDataHelper::GetDataSyncKeyRequestHandle(TStringBuf uid, EKey key) {
    auto request = CreateDataSyncRequest(ToDataSyncKey(Ctx, key));
    AddTVM2AuthorizationHeaders(*request.Get(), uid);
    return request->SetMethod("GET").Fetch();
}

TResultValue TPersonalDataHelper::GetDataSyncKeyValueFromResponse(TStringBuf uid, TStringBuf key,
                                                                  const NHttpFetcher::TResponse::TRef response,
                                                                  TString& value)
{
    if (response && response->IsError() && response->Code == HttpCodes::HTTP_NOT_FOUND) {
        return TError(TError::EType::NODATASYNCKEYFOUND);
    }

    const auto error = VerifyResponse(response, [uid, key](TStringBuilder& message) {
        message << "Failed to get DataSync value for UID: " << uid << ", KEY: " << key;
    } /* onError */);
    if (error) {
        return error;
    }

    Y_ASSERT(response);
    const NSc::TValue d = NSc::TValue::FromJson(response->Data);

    if (!d.Has("value")) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "DataSync response doesn't have value field: " << response->Data);
    }

    const auto& v = d["value"];
    if (!v.IsString()) {
        return TError(TError::EType::SYSTEM, TStringBuilder() << "DataSync response value is not string: " << response->Data);
    }

    value = v.GetString();
    return TResultValue{};
}

TResultValue TPersonalDataHelper::GetPrefetchedDataSyncKeyValue(TStringBuf key, TString& value) {
    Y_ASSERT(UseUniproxyDatasyncProtocol);

    const auto& jsonRawValue = PersonalData.Get(key);
    if (jsonRawValue.IsNull()) {
        return TError(TError::EType::NODATASYNCKEYFOUND, TStringBuilder() << "Can't find the requested key \"" << key << "\"");
    }

    if (!jsonRawValue.IsString()) {
        return TError(TError::EType::SYSTEM, TStringBuilder() << "DataSync response value is not string: " << jsonRawValue.ToJsonPretty());
    }

    value = jsonRawValue.GetString();
    return TResultValue();
}

TResultValue TPersonalDataHelper::GetDataSyncKeyValue(TStringBuf uid, EKey key, TString& value) {
    if (UseUniproxyDatasyncProtocol) {
        return GetPrefetchedDataSyncKeyValue(ToDataSyncKey(Ctx, key), value);
    }

    const auto response = GetDataSyncKeyRequestHandle(uid, key)->Wait();
    return GetDataSyncKeyValueFromResponse(uid, ToDataSyncKey(Ctx, key), response, value);
}

TSavedAddress TPersonalDataHelper::GetNavigatorUserAddress(TStringBuf addressId, TStringBuf searchText) const {
    const auto& navigatorState = Ctx.Meta().DeviceState().NavigatorState();
    TMaybe<TUserBookmark> naviUserAddress;

    TSpecialLocation locationType(addressId);

    switch (locationType) {
    case TSpecialLocation::EType::HOME:
        if (!navigatorState.Home().IsNull()) {
            naviUserAddress = TUserBookmark(navigatorState.Home());
        }
        break;

    case TSpecialLocation::EType::WORK:
        if (!navigatorState.Work().IsNull()) {
            naviUserAddress = TUserBookmark(navigatorState.Work());
        }
        break;

    default:
        LOG(DEBUG) << "Invalid address id <" << addressId << "> to obtain address from Yandex.Navigator state" << Endl;
        return TSavedAddress();
    }

    // try to find home or work in user bookmarks
    if (!naviUserAddress) {
        naviUserAddress = Ctx.GetUserBookmarksHelper()->GetSavedAddressBookmark(TSpecialLocation(addressId), searchText);
    }

    if (!naviUserAddress) {
        LOG(DEBUG) << "Failed to obtain <" << addressId << "> user address from Yandex.Navigator state" << Endl;
        return TSavedAddress();
    }

    TSavedAddress userAddress;
    userAddress.FromGeoPos(addressId, TGeoPosition(naviUserAddress->Lat(), naviUserAddress->Lon()), Ctx.Meta().Epoch());

    return userAddress;
}

TResultValue TPersonalDataHelper::SaveNavigatorUserAddress(TStringBuf addressId, const TSavedAddress& address) const {
    TSaveAddressNavigatorIntent navigatorIntent(Ctx, addressId, TGeoPosition(address.Latitude(), address.Longitude()));
    return navigatorIntent.Do();
}

// static
bool TPersonalDataHelper::CleanupTestUserKolonkish(TContext& /* ctx */) {
    // TODO implement it
    return true;
}

NSc::TValue TPersonalDataHelper::PrepareDataSyncBatchRequestContent(const TVector<TKeyValue>& kvs) {
    // Format is described here:
    // https://wiki.yandex-team.ru/disk/mpfs/platformapibatchrequests/
    NSc::TValue request;

    NSc::TArray& items = request["items"].GetArrayMutable();

    for (const auto& kv : kvs) {
        NSc::TValue body;
        body["value"] = kv.Value;

        NSc::TValue item;
        item["method"] = "PUT";
        item["relative_url"] = ToDataSyncKey(Ctx, kv.Key);
        item["body"] = body.ToJson();

        items.push_back(item);
    }

    return request;
}

void TPersonalDataHelper::AddTVM2AuthorizationHeaders(NHttpFetcher::TRequestBuilder& builder,
                                                      bool isAuthorized) const {
    TString serviceTicket;
    if (!GetTVM2ServiceTicket(Ctx.GetConfig().Vins().PersonalData().Tvm2ClientId(), serviceTicket)) {
        return;
    }

    TString userTicket;
    if (!isAuthorized) {
        builder.AddHeader("X-Uid", TStringBuilder() << "device_id:" << Ctx.Meta().UUID());
    } else if (GetTVM2UserTicket(userTicket)) {
        builder.AddHeader("X-Ya-User-Ticket", userTicket);
    }
}

void TPersonalDataHelper::AddTVM2AuthorizationHeaders(NHttpFetcher::TRequestBuilder& builder, TStringBuf uid) const {
    TString ticket;
    if (!GetTVM2ServiceTicket(Ctx.GetConfig().Vins().PersonalData().Tvm2ClientId(), ticket)) {
        return;
    }

    builder.AddHeader("X-Uid", uid);
}

void TPersonalDataHelper::AddTVM2AuthorizationHeaders(NHttpFetcher::TRequest& request, TStringBuf uid) const {
    NHttpFetcher::TRequestBuilder builder{request};
    return AddTVM2AuthorizationHeaders(builder, uid);
}

void TPersonalDataHelper::AddTVM2AuthorizationHeaders(NHttpFetcher::TRequest& request, bool isAuthorized) const {
    NHttpFetcher::TRequestBuilder builder{request};
    return AddTVM2AuthorizationHeaders(builder, isAuthorized);
}

// static
TResultValue TPersonalDataHelper::VerifyResponse(const NHttpFetcher::TResponse::TRef response,
                                                 const std::function<void(TStringBuilder&)>& fn)
{
    TMessageBuilder msg(fn);
    if (!response || response->IsError()) {
        if (response)
            msg.Append(", reason: ").Append(response->GetErrorText());
        msg.Log();
        return TError(TError::EType::SYSTEM, response ? response->GetErrorText() : TString());
    }

    return TResultValue{};
}

// static
TResultValue TPersonalDataHelper::VerifyBatchResponse(const NHttpFetcher::TResponse::TRef response,
                                                      const std::function<void(TStringBuilder&)>& fn) {
    TMessageBuilder msg(fn);
    if (const auto error = VerifyResponse(response, fn)) {
        return error;
    }
    auto responseData = NSc::TValue::FromJson(response->Data);

    if (!responseData.Has("items")) {
        msg.Append("Unexpected format: response data has no items").Log();
        return TError(TError::EType::SYSTEM);
    }

    const auto& items = responseData.Get("items");

    if (!items.IsArray()) {
        msg.Append("Unexpected format: items is not an array").Log();
        return TError(TError::EType::SYSTEM);
    }

    TResultValue result;

    for (size_t requestNumber = 0; requestNumber < items.ArraySize(); ++requestNumber) {
        const auto& item = items.Get(requestNumber);
        if (!item.Has("code")) {
            msg.Append("Unexpected format: item has no code").Log();
            return TError(TError::EType::SYSTEM);
        }
        auto code = item.Get("code");
        if (!code.IsIntNumber()) {
            msg.Append("Unexpected format: item code is not a number").Log();
            return TError(TError::EType::SYSTEM);
        }
        i64 codeNumber = code.GetIntNumber();
        if (IsUserError(codeNumber) || IsServerError(codeNumber)) {
            msg.Append("Error in ")
                .Append(ToString(requestNumber))
                .Append(" request, code ")
                .Append(ToString(codeNumber))
                .Log();
            return TError(TError::EType::SYSTEM);
        }
    }

    return TResultValue{};
}

} // namespace NBASS

template <>
void Out<NBASS::TPersonalDataHelper::EKey>(IOutputStream& out, const NBASS::TPersonalDataHelper::EKey& key) {
    out << std::visit(NBASS::TKeyToStringConverter(), key);
}

template <>
void Out<NBASS::TPersonalDataHelper::TKeyValue>(IOutputStream& out, const NBASS::TPersonalDataHelper::TKeyValue& kv) {
    out << kv.Key << ": " << kv.Value;
}
