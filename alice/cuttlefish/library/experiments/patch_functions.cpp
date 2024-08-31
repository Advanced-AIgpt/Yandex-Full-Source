#include "patch_functions.h"
#include "session_context_proxy.h"
#include "utils.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json_value.h>

#include <util/string/split.h>

#include <algorithm>


namespace NVoice::NExperiments {

namespace {

TString GetEventType(const NJson::TJsonValue& event) {
    TString res = event["header"]["namespace"].GetString() + "." + event["header"]["name"].GetString();
    res.to_lower();
    return res;
}

NJson::TJsonValue::TMapType& EnsureMap(NJson::TJsonValue* x)
{
    return x->SetType(NJson::JSON_MAP).GetMapSafe();
}

inline bool IsValueInArray(const NJson::TJsonValue& val, const NJson::TJsonValue::TArray& array) {
    for (const NJson::TJsonValue& x : array) {
        if (x == val)
            return true;
    }
    return false;
}

template <class TIterator>
NJson::TJsonValue& GetValueByPath(NJson::TJsonValue& x, TIterator pathIt, const TIterator pathEnd)
{
    NJson::TJsonValue* cur = &x;
    while (pathIt != pathEnd) {
        cur = &EnsureMap(cur).try_emplace(*pathIt).first->second;
        ++pathIt;
    }
    return *cur;
}

template <class TPath>
void SetByPath(NJson::TJsonValue& root, const TPath& path, const NJson::TJsonValue& value, bool overwrite=true)
{
    NJson::TJsonValue& dst = GetValueByPath(root, path.cbegin(), path.cend());
    if (overwrite || dst.GetType() == NJson::JSON_UNDEFINED)
        dst = value;
}

template <class TPath>
void AppendByPath(NJson::TJsonValue& root, const TPath& path, const NJson::TJsonValue& value)
{
    NJson::TJsonValue& dst = GetValueByPath(root, path.cbegin(), path.cend());
    if (dst.GetType() == NJson::JSON_UNDEFINED) {
        dst.AppendValue(value);  // will turn `dst` to Array
    } else if (dst.IsArray() && !IsValueInArray(value, dst.GetArray())) {
        dst.AppendValue(value);
    }
}

template <class TPath>
void ExtendByPath(NJson::TJsonValue& root, const TPath& path, const NJson::TJsonValue& values)
{
    NJson::TJsonValue& dst = GetValueByPath(root, path.cbegin(), path.cend());
    if (dst.GetType() == NJson::JSON_UNDEFINED) {
        dst = values;
    } else if (dst.IsArray()) {
        for (const NJson::TJsonValue& value : values.GetArray()) {
            if (!IsValueInArray(value, dst.GetArray()))
                dst.AppendValue(value);
        }
    }
}

template <class TPath>
void DelByPath(NJson::TJsonValue& root, const TPath& path)
{
    Y_ENSURE(!path.empty());

    NJson::TJsonValue* cur = &root;

    auto it = path.cbegin();
    const auto last = path.cend() - 1;
    while (it != last) {
        if (!cur->IsMap())
            return;
        cur = &(*cur)[*(it++)];
    }

    if (cur->IsMap())
        cur->GetMapSafe().erase(*last);
}

void ExtendRequestExperiments(NJson::TJsonValue& root, const NJson::TJsonValue::TMapType& src)
{
    static const auto PATH = {"payload", "request", "experiments"};

    NJson::TJsonValue::TMapType& dst = GetValueByPath(root, PATH.begin(), PATH.end()).SetType(NJson::JSON_MAP).GetMapSafe();
    for (const auto& val : src)
        dst.try_emplace(val.first, val.second);
}

inline const TString* GetSessionContextField(const NAliceProtocol::TSessionContext& sessionContext, TSessionContextGetter getter) {
    return getter ? &(*getter)(sessionContext) : nullptr;
}

} // anoynmous namespace
// ------------------------------------------------------------------------------------------------

TSet::TSet(const TString& path, const NJson::TJsonValue& value)
    : Path(StringSplitter("payload" + path).Split('.'))
    , Value(value)
{ }

bool TSet::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const {
    SetByPath(event, Path, Value, Overwrite);
    return true;
}

// ------------------------------------------------------------------------------------------------

TSetIfNone::TSetIfNone(const TString& path, const NJson::TJsonValue& value)
    : TSet(path, value)
{
    Overwrite = false;
}

// ------------------------------------------------------------------------------------------------

TAppend::TAppend(const TString& path, const NJson::TJsonValue& value)
{
    if (path == ".request.experiments") {
        Path = StringSplitter("payload" + path + "." + value.GetStringSafe()).Split('.');
        Value = NJson::TJsonValue("1");
        Tricky = true;
    } else {
        Path = StringSplitter("payload" + path).Split('.');
        Value = value;
    }
}

bool TAppend::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const
{
    if (Tricky) {
        SetByPath(event, Path, Value, /* overwrite = */ false);
    } else {
        AppendByPath(event, Path, Value);
    }
    return true;
}

// ------------------------------------------------------------------------------------------------

TExtend::TExtend(const TString& path, const NJson::TJsonValue& value)
    : Path()
    , Value(value)
{
    if (path == ".request.experiments") {
        Y_ENSURE(value.IsArray() || value.IsMap(), "Value for TExtend must be Array or Map");
        Y_ENSURE(EnsureVinsExperimentsFormat(&Value));  // now `Value` is a Map
        Tricky = true;
    } else {
        Y_ENSURE(value.IsArray(), "Value for TExtend must be Array or Map");
    }

    Path = StringSplitter("payload" + path).Split('.');
}

bool TExtend::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const
{
    if (Tricky) {
        ExtendRequestExperiments(event, Value.GetMap());
    } else {
        ExtendByPath(event, Path, Value);
    }
    return true;
}

// ------------------------------------------------------------------------------------------------

TDel::TDel(const TString& path)
{
    Y_ENSURE(!path.empty(), "Path must not be empty for TDel");
    Path = StringSplitter("payload" + path).Split('.');
}

bool TDel::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const
{
    DelByPath(event, Path);
    return true;
}

// ------------------------------------------------------------------------------------------------

TImportMacro::TImportMacro(const TExpContext& expContext, const TString& path, const TString& macroName)
    : TExtend(path, expContext.Macros.at(macroName))
{ }

// ------------------------------------------------------------------------------------------------

TIfEventType::TIfEventType(const TString& eventType)
    : EventType(eventType)
{
    EventType.to_lower();
}

bool TIfEventType::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const {
    return GetEventType(event) == EventType;
}

// ------------------------------------------------------------------------------------------------

bool TIfHasStaffLogin::Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext& sessionContext) const {
    return !GetStaffLogin(sessionContext).empty();
}

// ------------------------------------------------------------------------------------------------

TIfStaffLoginEq::TIfStaffLoginEq(const TString& login)
    : StaffLogin(login)
{ }

bool TIfStaffLoginEq::Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext& sessionContext) const {
    return GetStaffLogin(sessionContext) == StaffLogin;
}

// ------------------------------------------------------------------------------------------------

TIfStaffLoginIn::TIfStaffLoginIn(const NJson::TJsonValue::TArray& logins)
{
    for (const NJson::TJsonValue& login : logins) {
        const auto res = StaffLogins.insert(login.GetStringSafe());
        Y_ENSURE(res.second, "Login '" << login.GetStringSafe() << "' is duplicated");
    }
}

bool TIfStaffLoginIn::Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext& sessionContext) const {
    return StaffLogins.contains(GetStaffLogin(sessionContext));
}

// ------------------------------------------------------------------------------------------------

TIfHasPayload::TIfHasPayload(const TString& path)
    : SessionContextGetter(GetSessionContextGetter(path))
    , Path(StringSplitter("payload" + path).Split('.'))
{ }

bool TIfHasPayload::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& sessionContext) const {
    return GetJsonValueByPath(event, Path) || GetSessionContextField(sessionContext, SessionContextGetter);
}

// ------------------------------------------------------------------------------------------------

TIfPayloadEq::TIfPayloadEq(const TString& path, const NJson::TJsonValue& value)
    : SessionContextGetter(value.IsString() ? GetSessionContextGetter(path) : nullptr)
    , Path(StringSplitter("payload" + path).Split('.'))
    , Value(value)
{ }

bool TIfPayloadEq::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& sessionContext) const {
    if (const NJson::TJsonValue* val = GetJsonValueByPath(event, Path)) {
        return Value == *val;
    }
    if (const TString* val = GetSessionContextField(sessionContext, SessionContextGetter)) {
        return Value.GetString() == *val;
    }
    return false;
}

// ------------------------------------------------------------------------------------------------

TIfPayloadNe::TIfPayloadNe(const TString& path, const NJson::TJsonValue& value)
    : SessionContextGetter(value.IsString() ? GetSessionContextGetter(path) : nullptr)
    , Path(StringSplitter("payload" + path).Split('.'))
    , Value(value)
{ }

bool TIfPayloadNe::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& sessionContext) const {
    if (const NJson::TJsonValue* val = GetJsonValueByPath(event, Path)) {
        return Value != *val;
    }
    if (const TString* val = GetSessionContextField(sessionContext, SessionContextGetter)) {
        return Value.GetString() != *val;
    }
    return true;
}

// ------------------------------------------------------------------------------------------------

TIfPayloadIn::TIfPayloadIn(const TString& path, const NJson::TJsonValue::TArray& values)
    : SessionContextGetter(GetSessionContextGetter(path))
    , Path(StringSplitter("payload" + path).Split('.'))
    , Values(values)
{ }

bool TIfPayloadIn::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& sessionContext) const {
    if (const NJson::TJsonValue* val = GetJsonValueByPath(event, Path)) {
        return IsValueInArray(*val, Values);
    }
    if (const TString* val = GetSessionContextField(sessionContext, SessionContextGetter)) {
        return IsValueInArray(*val, Values);
    }
    return false;
}

// ------------------------------------------------------------------------------------------------

TIfPayloadLike::TIfPayloadLike(const TString& path, const TString& regExpr)
    : SessionContextGetter(GetSessionContextGetter(path))
    , Path(StringSplitter("payload" + path).Split('.'))
    , Regex(regExpr)
{ }

bool TIfPayloadLike::Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& sessionContext) const {
    if (const NJson::TJsonValue* val = GetJsonValueByPath(event, Path)) {
        return val->IsString() && Regex.Match(val->GetStringSafe().Data());
    }
    if (const TString* val = GetSessionContextField(sessionContext, SessionContextGetter)) {
        return Regex.Match(val->Data());
    }
    return false;
}

// ------------------------------------------------------------------------------------------------

TIfHasSessionData::TIfHasSessionData(const TString& path)
    : SessionContextGetter(GetSessionContextGetter(path))
    , Path(StringSplitter("payload" + path).Split('.'))
{ }

bool TIfHasSessionData::Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext& sessionContext) const {
    return GetSessionContextField(sessionContext, SessionContextGetter);
}

// ------------------------------------------------------------------------------------------------

TIfSessionDataEq::TIfSessionDataEq(const TString& path, const NJson::TJsonValue& value)
    : SessionContextGetter(value.IsString() ? GetSessionContextGetter(path) : nullptr)
    , Path(StringSplitter("payload" + path).Split('.'))
    , Value(value)
{ }

bool TIfSessionDataEq::Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext& sessionContext) const {
    if (const TString* val = GetSessionContextField(sessionContext, SessionContextGetter)) {
        return Value.GetString() == *val;
    }
    return false;
}

// ------------------------------------------------------------------------------------------------

TIfSessionDataNe::TIfSessionDataNe(const TString& path, const NJson::TJsonValue& value)
    : SessionContextGetter(value.IsString() ? GetSessionContextGetter(path) : nullptr)
    , Path(StringSplitter("payload" + path).Split('.'))
    , Value(value)
{ }

bool TIfSessionDataNe::Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext& sessionContext) const {
    if (const TString* val = GetSessionContextField(sessionContext, SessionContextGetter)) {
        return Value.GetString() != *val;
    }
    return true;
}

// ------------------------------------------------------------------------------------------------

TIfSessionDataIn::TIfSessionDataIn(const TString& path, const NJson::TJsonValue::TArray& values)
    : SessionContextGetter(GetSessionContextGetter(path))
    , Path(StringSplitter("payload" + path).Split('.'))
    , Values(values)
{ }

bool TIfSessionDataIn::Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext& sessionContext) const {
    if (const TString* val = GetSessionContextField(sessionContext, SessionContextGetter)) {
        return IsValueInArray(*val, Values);
    }
    return false;
}

// ------------------------------------------------------------------------------------------------

TIfSessionDataLike::TIfSessionDataLike(const TString& path, const TString& regExpr)
    : SessionContextGetter(GetSessionContextGetter(path))
    , Path(StringSplitter("payload" + path).Split('.'))
    , Regex(regExpr)
{ }

bool TIfSessionDataLike::Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext& sessionContext) const {
    if (const TString* val = GetSessionContextField(sessionContext, SessionContextGetter)) {
        return Regex.Match(val->Data());
    }
    return false;
}

// ------------------------------------------------------------------------------------------------

NJson::TJsonValue::TArray ExtractUaasTestsFromMegamindCookie(const NJson::TJsonValue& request) {
    if (request.IsMap()) {
        auto& requestMap = request.GetMapSafe();
        if (auto* cookiesJson = requestMap.FindPtr("megamind_cookies")) {
            if (NJson::TJsonValue cookieValue; NJson::ReadJsonTree(cookiesJson->GetString(), &cookieValue, false)) {
                const auto& cookieMap = cookieValue.GetMap();
                if (const auto* testsList = cookieMap.FindPtr("uaas_tests")) {
                    return testsList->GetArray();
                }
            }
        }
    }
    return {};
}


bool TransferUaasTestsFromMegamindCookie(NJson::TJsonValue& request) {
    using namespace NJson;
    const TJsonValue::TArray tests = ExtractUaasTestsFromMegamindCookie(request);
    if (tests) {
        TJsonValue::TArray& uaasTests = request["uaas_tests"].SetType(EJsonValueType::JSON_ARRAY).GetArraySafe();
        uaasTests.insert(uaasTests.end(), tests.begin(), tests.end());
        return true;
    }
    return false;
}

} // namespace NVoice::NExperiments
