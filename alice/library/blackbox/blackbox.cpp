#include "blackbox.h"

#include <alice/library/network/headers.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/ascii.h>
#include <util/string/join.h>

namespace NAlice {
namespace {

constexpr TStringBuf PHONE_ATTR_E164_NUMBER = "102";
constexpr TStringBuf PHONE_ATTR_IS_DEFAULT = "107";
constexpr TStringBuf PHONE_ATTR_IS_MAIN = "108";
constexpr TStringBuf ALIAS_IS_STAFF = "13";
constexpr TStringBuf ATTR_IS_BETA_TESTER = "8";
constexpr TStringBuf ATTR_YANDEX_PLUS = "1015";
const TString PHONE_ATTRIBUTES = Join(",", PHONE_ATTR_IS_DEFAULT, PHONE_ATTR_E164_NUMBER, PHONE_ATTR_IS_MAIN);

TBlackBoxErrorOr<NSc::TValue> ParseResponse(TStringBuf content) {
    try {
        auto json = NSc::TValue::FromJsonThrow(content);
        if (!json["exception"].IsNull()) {
            return TBlackBoxError{EBlackBoxErrorCode::BadData} << json;
        }
        return json;
    } catch (const NSc::TSchemeParseException& e) {
        return TBlackBoxError{EBlackBoxErrorCode::BadData} << "Failed to parse Blackbox json response: " << e.AsStrBuf() << ", content: " << content;
    } catch (...) {
        return TBlackBoxError{EBlackBoxErrorCode::BadData} << "Failed to parse BlackBox response: " << content;
    }
}

TBlackBoxErrorOr<TString> ParseUidImpl(const NSc::TValue& response) {
    if (const TStringBuf uid = response.TrySelect("uid/value").GetString(); !uid.Empty()) {
        return TString{uid};
    }

    return TBlackBoxError{EBlackBoxErrorCode::NoUid} << "No uid in BlackBox response: " << response.ToJson();
}

TBlackBoxErrorOr<TString> ParseUserTicket(const NSc::TValue& response) {
    if (const TStringBuf userTicket = response.TrySelect("user_ticket").GetString(); !userTicket.Empty()) {
        return TString{userTicket};
    }

    return TBlackBoxError{EBlackBoxErrorCode::NoUserTicket} << "No user ticket in BlackBox response: " << response.ToJson();
}

template <typename TOnResult>
bool ParseStringByPath(const NSc::TValue& data, TStringBuf path, TOnResult&& onResult) {
    const NSc::TValue& node = data.TrySelect(path);
    if (!node.StringEmpty()) {
        onResult(node.GetString());
        return true;
    }
    return false;
}

template <typename TOnResult>
bool ParseEmail(const NSc::TValue& data, TOnResult&& onResult) {
    return ParseStringByPath(data, TStringBuf("address-list/0/address"), onResult);
}

template <typename TOnResult>
bool ParseFirstName(const NSc::TValue& data, TOnResult&& onResult) {
    return ParseStringByPath(data, TStringBuf("dbfields/userinfo.firstname.uid"), onResult);
}

template <typename TOnResult>
bool ParseLastName(const NSc::TValue& data, TOnResult&& onResult) {
    return ParseStringByPath(data, TStringBuf("dbfields/userinfo.lastname.uid"), onResult);
}

template <typename TOnResult>
bool ParsePhone(const NSc::TValue& data, TOnResult&& onResult) {
    TString phone;
    const auto& phones = data["phones"].GetArray();
    for (const auto& phoneData : phones) {
        const auto& attrs = phoneData["attributes"];
        if (attrs.IsNull()) {
            continue;
        }

        auto isAttributeSet = [&attrs](TStringBuf name) {
            return attrs[name].GetString() == TStringBuf("1");
        };

        const bool isMain = isAttributeSet(PHONE_ATTR_IS_MAIN);
        if (phone.Empty() || isAttributeSet(PHONE_ATTR_IS_DEFAULT) || isMain) {
            phone = attrs[PHONE_ATTR_E164_NUMBER].GetString();
            if (isMain) {
                break;
            }
        }
    }

    if (phone.Empty()) {
        return false;
    }

    onResult(phone);
    return true;
}

template <typename T>
void ParseIsStaff(const NSc::TValue& data, T&& onValue) {
    onValue(data["aliases"].Has(ALIAS_IS_STAFF));
}

template <typename T>
void ParseIsBetaTester(const NSc::TValue& data, T&& onValue) {
    onValue(data["attributes"][ATTR_IS_BETA_TESTER].GetString() == "1");
}

template <typename T>
void ParseYandexPlus(const NSc::TValue& data, T&& onValue) {
    onValue(data["attributes"][ATTR_YANDEX_PLUS].GetString() == "1");
}

template <typename T>
void ParseHasMusicSubscription(const NSc::TValue& data, T&& onValue) {
    onValue(data["billing_features"].Has("basic-music"));
}

template <typename T>
void ParseMusicSubscriptionRegionId(const NSc::TValue& data, T&& onValue) {
    if (data["billing_features"].Has("basic-music") && data["billing_features"]["basic-music"].Has("region_id")) {
        onValue(data["billing_features"]["basic-music"]["region_id"].GetNumber());
    }
}

template <typename TOnResult>
void ParseSubscriptions(const NSc::TValue& data, TOnResult&& onValue) {
    for (const auto& key : data["billing_features"].DictKeys()) {
        onValue(key);
    }
}

template <typename TOnResult>
bool ParseLoginId(const NSc::TValue& data, TOnResult&& onResult) {
    return ParseStringByPath(data, "login_id", onResult);
}

} // namespace

TBlackBoxStatus PrepareBlackBoxRequest(NNetwork::IRequestBuilder& request,
                                       TStringBuf userIp, const TMaybe<TString>& authToken,
                                       TBlackBoxPrepareParams params)
{
    if (userIp.Empty()) {
        return TBlackBoxError{EBlackBoxErrorCode::NoUserIP};
    }

    if (authToken.Empty() && params.HasFlags(EBlackBoxPrepareParam::NeedAuthToken)) {
        return TBlackBoxError{EBlackBoxErrorCode::NoAuthToken};
    }

    request.AddCgiParam(TStringBuf("method"), TStringBuf("oauth"));
    request.AddCgiParam(TStringBuf("format"), TStringBuf("json"));
    request.AddCgiParam(TStringBuf("userip"), userIp);

    if (params.HasFlags(EBlackBoxPrepareParam::NeedUserInfo)) {
        request.AddCgiParam(TStringBuf("emails"), TStringBuf("getdefault"));
        request.AddCgiParam(TStringBuf("dbfields"), TStringBuf("userinfo.firstname.uid,userinfo.lastname.uid"));
        request.AddCgiParam(TStringBuf("getphones"), TStringBuf("bound"));
        request.AddCgiParam(TStringBuf("phone_attributes"), PHONE_ATTRIBUTES);
        request.AddCgiParam(TStringBuf("attributes"),  Join(",", ATTR_YANDEX_PLUS, ATTR_IS_BETA_TESTER));
        request.AddCgiParam(TStringBuf("aliases"), ALIAS_IS_STAFF);
        request.AddCgiParam(TStringBuf("get_billing_features"), TStringBuf("all"));
        request.AddCgiParam(TStringBuf("get_login_id"), TStringBuf("yes"));
    }

    if (params.HasFlags(EBlackBoxPrepareParam::NeedTicket)) {
        request.AddCgiParam(TStringBuf("get_user_ticket"), TStringBuf("yes"));
    }

    if (authToken.Defined()) {
        constexpr TStringBuf prefix = "oauth ";
        TString token = *authToken;
        if (!AsciiHasPrefixIgnoreCase(token, prefix)) {
            token = TString::Join(prefix, token);
        }
        request.AddHeader(NNetwork::HEADER_AUTHORIZATION, token);
    }

    return BlackBoxSuccess();
}

TBlackBoxErrorOr<TBlackBoxApi::TFullUserInfo> TBlackBoxApi::ParseFullInfo(TStringBuf content) const {
    TMaybe<NSc::TValue> response;
    auto setResponse = [&response](NSc::TValue&& r) {
        response = std::move(r);
    };
    if (auto e = ParseResponse(content).OnResult(setResponse)) {
        return std::move(*e);
    }

    Y_ASSERT(response.Defined());

    TFullUserInfo fullInfo;
    auto& info = *fullInfo.MutableUserInfo();

    auto setUid = [&info](TString&& uid) {
        info.SetUid(std::move(uid));
    };
    if (auto e = ParseUidImpl(*response).OnResult(setUid)) {
        return std::move(*e);
    }

    ParseEmail(*response, [&info](TStringBuf v) { info.SetEmail(TString{v}); });
    ParseFirstName(*response, [&info](TStringBuf v) { info.SetFirstName(TString{v}); });
    ParseLastName(*response, [&info](TStringBuf v) { info.SetLastName(TString{v}); });
    ParsePhone(*response, [&info](TStringBuf v) { info.SetPhone(TString{v}); });
    ParseIsStaff(*response, [&info](bool v) { info.SetIsStaff(v); });
    ParseIsBetaTester(*response, [&info](bool v) { info.SetIsBetaTester(v); });
    ParseYandexPlus(*response, [&info](bool v) { info.SetHasYandexPlus(v); });
    // FIXME We ignore error here and I don't know why.
    ParseUserTicket(*response).OnResult([&fullInfo](TString&& v) { fullInfo.SetUserTicket(std::move(v)); });
    ParseHasMusicSubscription(*response, [&info](bool v) { info.SetHasMusicSubscription(v); });
    ParseMusicSubscriptionRegionId(*response, [&info](ui64 v){ info.SetMusicSubscriptionRegionId(v); });
    ParseSubscriptions(*response, [&info](TStringBuf value) { info.AddSubscriptions(TString{value}); });
    ParseLoginId(*response, [&info](TStringBuf value) { info.SetLoginId(TString{value}); });

    return std::move(fullInfo);
}

TBlackBoxErrorOr<TString> TBlackBoxApi::ParseUid(TStringBuf content) const {
    return ParseResponse(content).AndThen(ParseUidImpl);
}

TBlackBoxErrorOr<TString> TBlackBoxApi::ParseTvm2UserTicket(TStringBuf content) const {
    return ParseResponse(content).AndThen(ParseUserTicket);
}

} // namespace NAlice
