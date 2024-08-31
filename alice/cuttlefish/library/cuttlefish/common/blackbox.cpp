#include "blackbox.h"
#include <library/cpp/json/json_reader.h>
#include <util/string/builder.h>
#include <util/string/cast.h>


namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

    enum class EAliases : unsigned {  // https://wiki.yandex-team.ru/passport/dbmoving/#tipyaliasov
        Yandexoid = 13
    };

    enum class EAttributes : unsigned { // https://docs.yandex-team.ru/authdevguide/concepts/DB_About#db-attributes
        AccountIsBetaTester = 8
    };

    const TString ATTRIBUTES_ACCOUNT_IS_BETA_TESTER = "attributes." + ToString((unsigned)EAttributes::AccountIsBetaTester);

}  // anonymous namespace

NAppHostHttp::THttpRequest TBlackboxClient::GetUidForSessionId(TStringBuf sessionId, TStringBuf userIp, TStringBuf origin)
{
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Get);
    req.SetScheme(NAppHostHttp::THttpRequest::Https);
    req.SetPath(TStringBuilder() << "?"
        "method=sessionid&"
        "format=json&"
        "get_user_ticket=yes&"
        "sessionid=" << sessionId << "&"
        "userip=" << userIp << "&"
        "aliases=" << unsigned(EAliases::Yandexoid) << "&"
        "attributes=" << unsigned(EAttributes::AccountIsBetaTester) << "&"
        "host=" << origin
    );
    return req;
}

NAppHostHttp::THttpRequest TBlackboxClient::GetUidForOAuth(TStringBuf token, TStringBuf userIp)
{
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Get);
    req.SetScheme(NAppHostHttp::THttpRequest::Https);
    req.SetPath(TStringBuilder() << "?"
        "method=oauth&"
        "format=json&"
        "get_user_ticket=yes&"
        "oauth_token=" << token << "&"
        "userip=" << userIp << "&"
        "attributes=" << unsigned(EAttributes::AccountIsBetaTester) << "&"
        "aliases=" << unsigned(EAliases::Yandexoid)
    );
    return req;
}

TBlackboxClient::TOAuthResponse TBlackboxClient::ParseResponse(TStringBuf response)
{
    NJson::TJsonValue value;
    if (!NJson::ReadJsonTree(response, &value, /*throwOnError = */ false))
        return {};
    if (value["status"]["value"] != "VALID")
        return {};

    bool isBetaTester = false;

    if (NJson::TJsonValue isBetaTesterValue; value.GetValueByPath(ATTRIBUTES_ACCOUNT_IS_BETA_TESTER, isBetaTesterValue)) {
        isBetaTester = (isBetaTesterValue.GetString() == "1");
    }

    return {
        /* Valid = */ true,
        /* Uid = */ value["oauth"]["uid"].GetString(),
        /* StaffLogin = */ value["aliases"][ToString((unsigned)EAliases::Yandexoid)].GetString(),
        /* UserTicket = */ value["user_ticket"].GetString(),
        /* IsBetaTester = */ isBetaTester
    };
}

TBlackboxResponseParser::TBlackboxResponseParser(TStringBuf itemType) : ItemType(itemType)
{
}

TMaybe<TBlackboxClient::TOAuthResponse> TBlackboxResponseParser::TryParse(
    const NAppHost::IServiceContext& ahContext,
    TSourceMetrics& metrics,
    TLogContext logContext
) const {
    if (!ahContext.HasProtobufItem(ItemType)) {
        logContext.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from Blackbox");
        metrics.PushRate("response", "noans", "blackbox");
        return Nothing();
    }

    const auto &httpResp = ahContext.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ItemType);
    logContext.LogEventInfoCombo<NEvClass::RecvFromAppHostBlackboxHttpResponse>(
            TStringBuilder() << "http_code=" << httpResp.GetStatusCode());

    // body with user token log only to local eventlog
    logContext.LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Blackbox response: " << httpResp));
    if (httpResp.GetStatusCode() != 200) {
        metrics.PushRate("response", "error", "blackbox");
        return Nothing();
    }

    TBlackboxClient::TOAuthResponse blackboxResp = TBlackboxClient::ParseResponse(httpResp.GetContent());
    if (blackboxResp.Valid) {
        metrics.PushRate("response", "ok", "blackbox");
        return blackboxResp;
    } else {
        metrics.PushRate("response", "invalid", "blackbox");
        logContext.LogEventInfoCombo<NEvClass::WarningMessage>("Blackbox rejected token");
        return Nothing();
    }
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
