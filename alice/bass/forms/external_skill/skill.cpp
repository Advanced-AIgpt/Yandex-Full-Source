#include "skill.h"

#include "ifs_map.h"
#include "parser1x.h"
#include "validator.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/video/utils.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/forms_db/forms_db.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/socialism/socialism.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/ner/ner.h>
#include <alice/bass/libs/tvm2/tvm2_ticket_cache.h>
#include <alice/library/unittest/fake_fetcher.h>

#include <kernel/translit/translit.h>

#include <library/cpp/json/writer/json.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/uri/uri.h>
#include <library/cpp/semver/semver.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/threading/future/async.h>

#include <util/charset/utf8.h>
#include <util/generic/algorithm.h>
#include <util/generic/guid.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/join.h>
#include <util/string/util.h>

#include <cmath>
#include <iomanip>
#include <sstream>

namespace NBASS {
namespace NExternalSkill {

namespace {

constexpr TStringBuf BILLING_SKILLS = "billing/createSkillPurchaseOffer";
constexpr TStringBuf COMMAND_CHAT_INFO = "update_dialog_info";
constexpr TStringBuf FS_SSL_CERT_ERROR = "FS_SSL_CERT_ERROR";
constexpr TStringBuf JSNAME_DEVELOPERMODE = "developer_mode";
constexpr TStringBuf SLOTNAME_SKILLINFO = "skill_info";

constexpr TStringBuf QUASAR_BILLING_SKILLS_PATH_1 = "skills/external/";
constexpr TStringBuf QUASAR_BILLING_SKILLS_PATH_2 = "/purchase_offer";
constexpr TStringBuf QUASAR_BILLING_SKILLS_OK = "skill_billing_start_purchase_offer_ok";
constexpr TStringBuf QUASAR_BILLING_SKILLS_OK_NO_PUSH = "skill_billing_start_purchase_offer_ok_no_push";
constexpr TStringBuf QUASAR_BILLING_SKILLS_FAIL = "skill_billing_start_purchase_offer_fail";
constexpr TStringBuf QUASAR_BILLING_SKILLS_PUSH_BODY = "Запрос покупки";
constexpr TStringBuf QUASAR_BILLING_SKILLS_SUCCESS = "skill_purchase_success";
constexpr TStringBuf QUASAR_BILLING_SKILLS_SUCCESS_DELEGATE = "skill_purchase_success_delegate_disclaimer";
constexpr TStringBuf QUASAR_BILLING_SKILLS_DEVICE_IS_NOT_SUPPORTED = "device_does_not_support_billing";

constexpr TStringBuf SKILL_ACCOUNT_LINKING_PATH = "store/account_linking/";
constexpr TStringBuf SKILL_ACCOUNT_LINKING_REQUIRED = "skill_account_linking_required";
constexpr TStringBuf SKILL_ACCOUNT_LINKING_REQUIRED_NO_PUSH = "skill_account_linking_required_no_push";
constexpr TStringBuf SKILL_ACCOUNT_LINKING_PUSH_BODY = "Запрос авторизации";
constexpr TStringBuf SKILL_ACCOUNT_LINKING_SUCCESS = "skill_account_linking_success";
constexpr TStringBuf SKILL_ACCOUNT_LINKING_SUCCESS_DELEGATE = "skill_account_linking_success_delegate_disclaimer";
constexpr TStringBuf SKILL_ACCOUNT_LINKING_DEVICE_IS_NOT_SUPPORTED = "device_does_not_support_account_linking";

constexpr ui32 PRECISION_PRICE = 2;
constexpr ui32 PRECISION_QUANTITY = 4;

const TString REQUEST_ERROR_HTTP = "request_error_http";
const TString REQUEST_ERROR_HTTP_SSL_CERT = "request_error_http_ssl_cert";
const TString REQUEST_ERROR_PARSE = "request_error_parse";
const TString REQUEST_ERROR_TIMEOUT = "request_error_timeout";
const TString REQUEST_NAME_RESOLUTION_ERROR = "request_name_resolution_error";

static TVector<TString> SKILL_RESPONSE_MAJOR_FIELDS = {
    "start_account_linking",
    "start_purchase"
};

const TString SKILL_RESPONSE_MINOR_FIELD = "response";

const std::array<TString, 2> STAT_NAME_API_REQUEST_TIME = {{
    TString("bass_skill_api_batch_request_time"),
    TString("bass_skill_api_request_time")
}};

const std::array<TString, 2> STAT_NAME_API_REQUEST_SUCCESS = {{
    TString("bass_skill_api_batch_request_success"),
    TString("bass_skill_api_request_success")
}};

const std::array<TString, 2> STAT_NAME_API_REQUEST_ERROR = {{
    TString("batch_bass_skill_api_request_error"),
    TString("bass_skill_api_request_error")
}};

bool IsSkillInDeveloperMode(const TContext& ctx, TStringBuf skillId) {
    const TSlot* const slot = ctx.GetSlot(SLOTNAME_SKILLINFO);
    if (IsSlotEmpty(slot) || !slot->Value[JSNAME_DEVELOPERMODE].GetBool(false)) {
        return ctx.Meta().Utterance() == skillId;
    }
    return true;
}

void LogWebhookError(const NHttpFetcher::TResponse::TRef resp) {
    NJson::TJsonValue error(NJson::JSON_MAP);
    NJson::TJsonValue headers(NJson::JSON_MAP);
    for (const auto& header : resp->Headers) {
        auto name = header.Name();
        name.to_lower();
        if (name.StartsWith("x-yandex")) {
            headers[header.Name()] = header.Value();
        }
    }
    error["headers"] = headers;
    error["status_code"] = resp->Code;
    error["response_body"] = resp->Data;
    NJsonWriter::TBuf buf;
    buf.WriteJsonValue(&error);
    LOG(ERR) << "Skill webhook fetch error: " << buf.Str() << Endl;
}

TErrorBlock::TResult GetSocialToken(TContext& ctx, TApiSkillResponseScheme::TResultConst skillDescr, TMaybe<TString>* oauth) {
    Y_ASSERT(oauth);

    TString socialismId = TString{*skillDescr.AccountLinking().ApplicationName()};
    if (socialismId.empty()) {
        return TErrorBlock::Ok;
    }

    TString uid;
    TPersonalDataHelper pdh(ctx);
    if (!pdh.GetUid(uid)) {
        // Uid in meta is obtained from dialogs testing tab (https://dialogs.yandex.ru/developer/skills/<skill_id>/draft/test)
        // Uid is sent with every request
        uid = ToString(ctx.Meta().UID());
        if (uid.empty()) {
            // This is ok because skill should be called without an Authorization header
            // which means that the user is not authorized,
            // and this is the skill responsibility to handle this situation.
            return TErrorBlock::Ok;
        }
    }

    TSocialismRequest req = FetchSocialismToken(ctx.GetSources().SocialApi().Request(), uid, socialismId);
    if (!req) {
        LOG(ERR) << "Skill requests socialism_token but unable to get request" << Endl;
        return TErrorBlock::Ok;
    }

    TString body;
    if (req->WaitAndParseResponse(&body) || !body) {
        LOG(ERR) << "Request to socialism failed" << Endl;
        return TErrorBlock::Ok;
    }

    oauth->ConstructInPlace(std::move(body));

    return TErrorBlock::Ok;
}

TString SkipNonAsciiSymbols(TStringBuf name) {
    TStringBuilder b;
    const ui32 len = GetNumberOfUTF8Chars(name);
    for (ui32 i = 0; i < len; ++i) {
        TStringBuf rune = SubstrUTF8(name, i, 1);
        if (UTF8Detect(rune) == EUTF8Detect::ASCII) {
            b << rune;
        }
    }
    return b;
}

bool IsValidSymbol(char c) {
    return
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '.' || c == '-' || c == '/' || c == '@' ||
        c == '_';
}

TString BuildSkillNameForMetrics(TStringBuf name) {
    const TString norm = TransliterateBySymbol(ToLowerUTF8(name), ELanguage::LANG_RUS);
    const TString ascii = SkipNonAsciiSymbols(norm);

    TStringBuilder b;
    bool toUpper = false;
    for (char c : ascii) {
        if (!IsValidSymbol(c)) {
            continue;
        }
        if (c == ' ') {
            toUpper = true;
            continue;
        }
        if (toUpper) {
            c = AsciiToUpper(c);
            toUpper = false;
        }
        b << c;
    }
    return b;
}

bool IsSslCertError(const TString& responseErrorText) {
    return responseErrorText.Contains(FS_SSL_CERT_ERROR);
}


////////// TSkillResolver //////////////////////////////////////////////////////////////////////////////////////////////
class TSkillResolver : public ISkillResolver {
public:
    TSkillResponsePtr ResolveSkillId(TContext& ctx, TStringBuf skillId, const TConfig& config,
                                     TErrorBlock::TResult* error) const override
    {
        TVector<TSkillResponsePtr> skills = ResolveSkillIds(ctx, {skillId}, config, error);
        if (skills.empty()) {
            return nullptr;
        }
        return THolder(skills.front().Release());
    }

    TVector<TSkillResponsePtr> ResolveSkillIds(TContext& ctx, const TVector<TStringBuf>& skillIds, const TConfig&,
                                               TErrorBlock::TResult* error) const override
    {
        if (skillIds.empty()) {
            return {};
        }

        const bool isBatch = skillIds.size() == 1;

        NSc::TValue requestJson;
        for (size_t i = 0; i < skillIds.size(); ++i) {
            NSc::TValue& json = requestJson.Push();
            json["jsonrpc"].SetString("2.0");
            json["method"].SetString("getSkill");
            json["id"].SetIntNumber(1);
            json["params"].SetArray().Push().SetString(skillIds[i]);
        }

        THolder<NHttpFetcher::TRequest> req = ctx.GetSources().ExternalSkillsApi().Request();
        req->SetBody(requestJson.ToJson(), TStringBuf("POST"));
        req->SetContentType(TStringBuf("application/json"));

        NHttpFetcher::TResponse::TRef resp;
        {
            Y_STATS_SCOPE_HISTOGRAM(STAT_NAME_API_REQUEST_TIME[isBatch]);
            resp = req->Fetch()->Wait();
        }

        NSc::TValue responseJson;
        if (resp->IsHttpOk()) {
            Y_STATS_INC_COUNTER(STAT_NAME_API_REQUEST_SUCCESS[isBatch]);
            LOG(DEBUG) << "SkillApi response: " << resp->Data << Endl;
            responseJson = NSc::TValue::FromJson(resp->Data);
        } else {
            LOG(ERR) << "SkillApi (" << isBatch << ") error answer: " << resp->GetErrorText() << Endl;
            Y_STATS_INC_COUNTER(STAT_NAME_API_REQUEST_ERROR[isBatch]);

            for (const auto& skillId : skillIds) {
                const NSc::TValue skillInfo = ctx.GlobalCtx().FormsDb().ExternalSkillsDb().SkillInfo(skillId);
                // skillInfo may be either Null, or {"error":{"code":1, "message":"Skill not found"}} or real skill info
                // First two situations will be dealt with in the following validation
                responseJson.Push(skillInfo);
            }
            LOG(INFO) << "SkillDb response: " << responseJson.ToJson() << Endl;
        }

        TVector<TSkillResponsePtr> result{Reserve(skillIds.size())};
        for (size_t i = 0; i < responseJson.ArraySize(); ++i) {
            TSkillResponsePtr skill = MakeHolder<TApiSkillResponse>(responseJson[i]);
            TSkillValidateHelper errCollector(skill->Scheme().Result().IsInternal());
            skill->Scheme().Validate("", false, std::ref(errCollector), &errCollector);
            if (!errCollector.Error) {
                result.emplace_back(std::move(skill));
            }
            if (errCollector.Error && error) {
                error->Swap(errCollector.Error); // todo: merge errors
            }
        }
        return result;
    }
};

////////// TDummySkillResolver /////////////////////////////////////////////////////////////////////////////////////////
class TDummySkillResolver : public ISkillResolver {
public:
    TSkillResponsePtr ResolveSkillId(TContext&, TStringBuf, const TConfig&, TErrorBlock::TResult* error) const override {
        if (error)
            error->ConstructInPlace(TError::EType::SKILLSERROR, "unknown");
        return nullptr;
    }

    TVector<TSkillResponsePtr> ResolveSkillIds(TContext&, const TVector<TStringBuf>&, const TConfig&,
                                               TErrorBlock::TResult* error) const override {
        if (error)
            error->ConstructInPlace(TError::EType::SKILLSERROR, "unknown");
        return {};
    }
};

TString GetSkillRequestSource(const TContext& ctx, bool isConsole) {
    const TSlot* reqSlot = ctx.GetSlot("request", "string");
    if (!IsSlotEmpty(reqSlot) && reqSlot->Value.GetString() == "ping") {
        return "ping";
    }
    if (isConsole) {
        return "console";
    }
    return "user";
}

void IncSuccessOrFailCounter(const TString& name, const NHttpFetcher::TResponse::TRef& resp, IGlobalContext& globalCtx) {
    if (!name.empty()) {
        if (resp->IsHttpOk()) {
            Y_STATS_INC_COUNTER("extskill_" + name + "_success");
        } else {
            Y_STATS_INC_COUNTER("extskill_" + name + "_fail");
        }
        TString histName = TStringBuilder() << "extskill_" << name << "_response_time";

        NMonitoring::GetHistogram(histName).Record(resp->Duration.MilliSeconds());
        if (const auto* signal = globalCtx.Counters().BassCounters().UnistatHistograms.FindPtr(histName); signal) {
            (*signal)->PushSignal(resp->Duration.MilliSeconds());
        }
    }
}

void UpdateSkillCounters(const TString& id,
                         const TString& skillNameNorm,
                         const TString& monitoringType,
                         const NHttpFetcher::TResponse::TRef& resp,
                         IGlobalContext& globalCtx)
{
    if (auto name = INTERNAL_SKILLS.FindPtr(id); name) {
        IncSuccessOrFailCounter("internal", resp, globalCtx);
        IncSuccessOrFailCounter(*name, resp, globalCtx);
    } else if (monitoringType == "yandex") {
        IncSuccessOrFailCounter(skillNameNorm, resp, globalCtx);
    } else {
        IncSuccessOrFailCounter("external", resp, globalCtx);
    }
    IncSuccessOrFailCounter(monitoringType, resp, globalCtx);
}

TString RoundDoubleAndCastToString(double num, ui32 precision) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << num;
    return TString(out.str());
}

void SendPush(TContext& ctx,
    const TString& uid,
    const TString& link,
    const TApiSkillResponseScheme::TResultConst& skillResult,
    TStringBuf pushBody)
{
    if (uid.empty()) {
        return;
    }

    NHttpFetcher::TRequestPtr request = ctx.GetSources().SupProvider().Request();
    NSc::TValue body;

    TStringBuilder receiver;
    receiver << "tag:uid=='" << uid << "' AND app_id IN ('ru.yandex.searchplugin', 'ru.yandex.searchplugin.dev', "
                                       "'ru.yandex.mobile', 'ru.yandex.mobile.inhouse')";

    body["receiver"].SetArray().Push().SetString(receiver);
    body["ttl"].SetIntNumber(360);

    NSc::TValue& data = body["data"];
    data["push_id"].SetString("alice_skill_account_linking");

    NSc::TValue& notification = body["notification"];
    notification["body"].SetString(pushBody);
    notification["link"].SetString(link);
    notification["title"].SetString(skillResult.Title());
    body["project"].SetString("bass");
    body["is_data_only"].SetBool(true);

    request->SetBody(body.ToJson(), TStringBuf("POST"));
    request->SetContentType(TStringBuf("application/json;charset=UTF-8"));
    request->AddHeader("Authorization", TStringBuilder{} << "OAuth " << ctx.GetConfig().PushHandler().SupProvider().Token());

    auto response = request->Fetch()->Wait();
    if (response->IsHttpOk()) {
        LOG(INFO) << "Skills push sent" << Endl;
        LOG(DEBUG) << response->Data << Endl;
        Y_STATS_INC_COUNTER("bass_send_sup_skill_push_success");
    } else {
        LOG(ERR) << "Failed to send skill push, error: " << response->GetErrorText() << Endl;
        Y_STATS_INC_COUNTER("bass_failed_to_send_sup_skill_push_error");
    }
}

void RequestQuasarBillingSkills(TContext& ctx,
    const TApiSkillResponseScheme::TResultConst& skillResult,
    const NSc::TValue& skillRequestJson,
    const NSc::TValue& skillResponseJson,
    const TString& uid)
{
    try {

        NSc::TValue billingRequest;

        billingRequest["skill"].SetDict();
        billingRequest["skill"]["callback_url"] = skillResult.BackendSettings().Uri();
        billingRequest["skill"]["id"] = skillResult.Id();
        billingRequest["skill"]["name"] = skillResult.Title();
        billingRequest["skill"]["image_url"] = TSkillDescription::CreateImageUrl(ctx,
                                                                                 skillResult.Logo().AvatarId(),
                                                                                 IMAGE_TYPE_MOBILE_LOGO,
                                                                                 AVATAR_NAMESPACE_SKILL_LOGO);
        billingRequest["user_id"] = skillRequestJson["session"]["user_id"];
        billingRequest["session_id"] = skillRequestJson["session"]["session_id"];
        billingRequest["purchase_request"] = skillResponseJson["start_purchase"].Clone();
        billingRequest["version"] = skillResponseJson["version"];

        for (auto& product : billingRequest["purchase_request"]["products"].GetArrayMutable()) {
            product["user_price"].SetString(RoundDoubleAndCastToString(product["user_price"].GetNumber(), PRECISION_PRICE));
            product["price"].SetString(RoundDoubleAndCastToString(product["price"].GetNumber(), PRECISION_PRICE));
            // quantity can have up to 4 digits after floating point
            product["quantity"].SetString(RoundDoubleAndCastToString(product["quantity"].GetNumber(), PRECISION_QUANTITY));
            if (product["image_id"].StringEmpty()) {
                product["image_url"].SetNull();
            } else {
                product["image_url"] = TSkillDescription::CreateImageUrl(ctx,
                                                                         product["image_id"],
                                                                         IMAGE_TYPE_SMALL,
                                                                         AVATAR_NAMESPACE_SKILL_IMAGE);
            }
            product.Delete("image_id");
        }

        LOG(INFO) << "Billing request: " << billingRequest << Endl;

        NHttpFetcher::TRequestPtr request = ctx.GetSources().QuasarBillingSkills(BILLING_SKILLS).Request();

        bool shouldSendPush = ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsElariWatch();
        request->AddCgiParam(TStringBuf("send_push"), "false");

        request->SetBody(billingRequest.ToJson(), TStringBuf("POST"));
        request->SetContentType(TStringBuf("application/json"));
        request->AddHeader("Authorization", ctx.UserAuthorizationHeader());
        NVideo::AddCodecHeadersIntoRequest(request, ctx);

        auto billingResponse = request->Fetch()->Wait();

        NSc::TValue billingResponseJson;
        try {
            billingResponseJson = NSc::TValue::FromJsonThrow(billingResponse->Data, NSc::TValue::TJsonOpts(NSc::TValue::TJsonOpts::EJsonOpts::JO_PARSER_STRICT_JSON));
        } catch (const NSc::TSchemeParseException& e) {
            LOG(ERR) << "Billing error: failed to parse response from QuasarBillingSkills. Error: " << e.what() << Endl;
            LOG(ERR) << "Billing response: " << billingResponse->Data << Endl;
            ctx.AddAttention(QUASAR_BILLING_SKILLS_FAIL);
            return;
        }

        if (!billingResponse->IsHttpOk()) {
            LOG(ERR) << "Billing error: " << billingResponse->GetErrorText() << Endl;
            LOG(ERR) << "Billing response: " << billingResponse->Data << Endl;
            ctx.AddAttention(QUASAR_BILLING_SKILLS_FAIL);
        } else if (billingResponseJson["order_id"].StringEmpty()) {
            LOG(ERR) << "Billing error: order_id is empty" << Endl;
            LOG(ERR) << "Billing response: " << billingResponse->Data << Endl;
            ctx.AddAttention(QUASAR_BILLING_SKILLS_FAIL);
        } else {
            LOG(INFO) << "Billing OK" << Endl;

            TCgiParameters cgi;
            cgi.InsertUnescaped("expectedUid", uid);
            cgi.InsertUnescaped("purchaseOfferUuid", billingResponseJson["order_id"].GetString());
            if (shouldSendPush) {
                cgi.InsertUnescaped("initial_device_id", ctx.Meta().DeviceState().DeviceId());
            }
            TStringBuilder url;
            url << ctx.GetConfig().Hosts().QuasarBillingSkills()
                << QUASAR_BILLING_SKILLS_PATH_1 << skillResult.Id() << QUASAR_BILLING_SKILLS_PATH_2 << '?'
                << cgi.Print();

            TCgiParameters yellowskinCgi;
            yellowskinCgi.InsertUnescaped("url", url);
            const TString link{TStringBuilder() << "yellowskin://?" << yellowskinCgi.Print()};

            if (!shouldSendPush) {
                NSc::TValue suggestJson;
                suggestJson["url"].SetString(link);
                ctx.AddSuggest("skill_billing_request_make_purchase_button", std::move(suggestJson));
                ctx.CreateSlot("skill_billing_request", "skill_billing_request", true);
                ctx.AddAttention(QUASAR_BILLING_SKILLS_OK_NO_PUSH);
            } else {
                SendPush(ctx, uid, link, skillResult, QUASAR_BILLING_SKILLS_PUSH_BODY);
                ctx.AddAttention(QUASAR_BILLING_SKILLS_OK);
            }
            ctx.AddStopListeningBlock();
        }
    } catch (yexception& e) {
        LOG(ERR) << "Billing error: " << e.what() << Endl;
        ctx.AddStopListeningBlock();
        ctx.AddAttention(QUASAR_BILLING_SKILLS_FAIL);
    }
}

void StartAccountLinking(TContext& ctx, const TApiSkillResponseScheme::TResultConst& skillResult, const TString& uid) {
    TStringBuilder link;
    if (!ctx.MetaClientInfo().IsYaBrowser()) {
        TStringBuilder url;
        url << ctx.GetConfig().Hosts().DialogsAuthorizationSkills()
            << SKILL_ACCOUNT_LINKING_PATH
            << skillResult.Id();
        if (ctx.MetaClientInfo().IsSmartSpeaker()) {
            if (!ctx.Meta().DeviceState().DeviceId()->empty()) {
                url << "?initial_device_id=" << ctx.Meta().DeviceState().DeviceId();
            } else if (!ctx.Meta().DeviceId()->empty()) {
                url << "?initial_device_id=" << ctx.Meta().DeviceId();
            } else {
                LOG(ERR) << "DeviceId is Missing !" << Endl;
                Y_STATS_INC_COUNTER("bass_account_linking_missing_device_id_error");
            }
        } else {
            url << "?session_type=" << (ctx.Meta().VoiceSession() ? "voice" : "text");
        }

        TCgiParameters cgi;
        cgi.InsertEscaped("url", url);
        link << "yellowskin://?" << cgi.Print();
    } else {
        link << ctx.GetConfig().Hosts().DialogsAuthorizationSkills()
             << SKILL_ACCOUNT_LINKING_PATH
             << skillResult.Id()
             << "?session_type=" << (ctx.Meta().VoiceSession() ? "voice" : "text");
    }

    ctx.AddStopListeningBlock();
    if (ctx.MetaClientInfo().IsYaBrowser() || ctx.MetaClientInfo().IsSearchApp()) {
        NSc::TValue data;
        data["url"] = link;
        ctx.AddSuggest("skill_account_linking_button", std::move(data));
        ctx.AddAttention(SKILL_ACCOUNT_LINKING_REQUIRED_NO_PUSH);
        return;
    }
    ctx.AddAttention(SKILL_ACCOUNT_LINKING_REQUIRED);
    SendPush(ctx, uid, link, skillResult, SKILL_ACCOUNT_LINKING_PUSH_BODY);
}

NHttpFetcher::TRequestPtr PrepareYaFunctionsRequest(TContext& ctx, const TString& functionId, const TDuration skillTimeout) {
    NHttpFetcher::TRequestPtr r = ctx.GetSources().YandexFunctions(functionId).Request();
    r->AddCgiParam(TStringBuf("integration"), TStringBuf("raw"));
    if (skillTimeout) {
        r->AddHeader("X-Functions-Timeout-Ms", ToString(skillTimeout.MilliSeconds()));
    }
    // TODO: (akhruslan) Remove direct request to TVM2TicketCache
    TMaybe<TString> serviceTicket = ctx.GlobalCtx().TVM2TicketCache()->GetTicket(ctx.GetConfig().Vins().YandexFunctions().Tvm2ClientId());
    r->AddHeader("X-Functions-Service-Ticket", *serviceTicket);
    return r;
}

NHttpFetcher::TRequestPtr PrepareZoraRequest(const TContext& ctx, const TApiSkillResponseScheme::TResultConst& skillResult, NHttpFetcher::TRequestOptions options) {
    NUri::TUri uri = NHttpFetcher::ParseUri(skillResult.BackendSettings().Uri());
    NHttpFetcher::TRequestPtr r = NHttpFetcher::Request(uri, options);
    if (skillResult.UseZora()) {
        ProxyRequestViaZora(ctx.GetConfig(), r.Get(), ctx.GlobalCtx().SourcesRegistryDelegate(), ctx.HasExpFlag(EXPERIMENTAL_FLAG_ENABLE_SKILLS_SSL_CHECK));
        const auto& webhookTimeout = ctx.GetConfig().Vins().ExternalSkillsApi().ZoraRequestTimeout();
        r->AddHeader("X-Yandex-Response-Timeout", ToString(webhookTimeout->MilliSeconds() / 1000.0));
    }
    return r;
}

bool ZoraReturnedTimeout(NHttpFetcher::TResponse::TRef response) {
    // zora returns HTTP 504 with "X-Yandex-Error: request timed out" header
    // when webhook response time exceeds X-Yandex-Response-Timeout value from request
    auto* yandexErrorHeader = response->Headers.FindHeader("X-Yandex-Error");
    return yandexErrorHeader && yandexErrorHeader->Value() == TStringBuf("request timed out");
}

bool IsZoraError(NHttpFetcher::TResponse::TRef response) {
    return (response->Headers.HasHeader("X-Yandex-Status") || response->Data.StartsWith("zora:")) && !ZoraReturnedTimeout(response);
}

} // namespace anonymous

NMonitoring::TCountersChanger Sensors() {
    return NMonitoring::TCountersChanger(NMonitoring::GetSensors());
}

NMonitoring::TCountersChanger SkillSensors(TContext& ctx) {
    return NMonitoring::TCountersChanger(ctx.GlobalCtx().Counters().SkillSensors());
}

std::unique_ptr<ISkillResolver> ISkillResolver::Global = std::make_unique<TSkillResolver>();

// static
const ISkillResolver& ISkillResolver::GlobalResolver() {
    Y_ASSERT(Global.get());
    return *Global;
}

// static
void ISkillResolver::ResetGlobalResolver(std::unique_ptr<ISkillResolver> resolver) {
    if (resolver) {
        Global = std::move(resolver);
    } else {
        Global = std::make_unique<TDummySkillResolver>();
    }
}

TMaybe<TNerRequester> NerRequest(const TContext& ctx) {
    const TSlot* reqSlot = ctx.GetSlot(TStringBuf("request"), TStringBuf("string"));
    if (IsSlotEmpty(reqSlot)) {
        return Nothing();
    }
    TStringBuf skillId;
    if (const TContext::TSlot* slotSkill = ctx.GetSlot(TStringBuf("skill_id")); !IsSlotEmpty(slotSkill)) {
       skillId = slotSkill->Value.GetString();
    }
    return TNerRequester{ctx.GetSources().NerApi().Request(), reqSlot->Value.GetString(), skillId};
}

void OnNerResponse(TMaybe<TNerRequester>&& nerRequester, TMaybe<NSc::TValue>& nerInfo) {
    if (!nerRequester.Defined()) {
        return;
    }
    if (const NSc::TValue* nerData = nerRequester->Response()) {
        nerInfo.ConstructInPlace(*nerData);
    }
}

THolder<TApiSkillResponse> ConstructData(const TSlot& slot, TContext& ctx,
                                                            TErrorBlock::TResult* result,
                                                            TMaybe<NSc::TValue>& nerInfo,
                                                            bool isConsole)
{
    const TSlot* const slotSkillTest = ctx.GetSlot("skill_description");
    if (IsSlotEmpty(slotSkillTest)) {
        auto& resolver = ISkillResolver::GlobalResolver();
        const TStringBuf skillId = slot.Value.GetString();

        TMaybe<TNerRequester> nerRequester;

        ISkillResolver::TSkillResponsePtr skillResponse = resolver.ResolveSkillId(
            ctx, skillId, ctx.GetConfig(), result
        );

        if (!skillResponse) {
            // create an invalid object for the back compatibility
            skillResponse = MakeHolder<TApiSkillResponse>();
        }

        if (skillResponse->Scheme().Result().UseNLU()) {
            nerRequester = NerRequest(ctx);
        }
        OnNerResponse(std::move(nerRequester), nerInfo);

        return skillResponse;
    }

    // FIXME make separate ISkillResolver; check if test is working

    if (GetSkillRequestSource(ctx, isConsole) != "ping") {
        // TODO make request only in case useNLU is turned off
        OnNerResponse(NerRequest(ctx), nerInfo);
    } else {
        NSc::TValue pingNerInfo;
        pingNerInfo["nlu"]["entities"].SetArray();
        pingNerInfo["nlu"]["tokens"].SetArray().Push().SetString("ping");
        nerInfo.ConstructInPlace(pingNerInfo);
    }

    NSc::TValue json;
    json["result"] = slotSkillTest->Value;
    json["onAir"].SetBool(true); // always enabled when it is from api console for checking

    return MakeHolder<TApiSkillResponse>(std::move(json));
}


TMaybe<TNerRequester> NerRequestWithFakeRequest(const TContext& ctx, NHttpFetcher::TRequest* fakeRequest) {
    const TSlot* reqSlot = ctx.GetSlot(TStringBuf("request"), TStringBuf("string"));
    if (IsSlotEmpty(reqSlot)) {
        return Nothing();
    }
    TStringBuf skillId;
    if (const TContext::TSlot* slotSkill = ctx.GetSlot(TStringBuf("skill_id")); !IsSlotEmpty(slotSkill)) {
       skillId = slotSkill->Value.GetString();
    }
    return TNerRequester{THolder(fakeRequest), reqSlot->Value.GetString(), skillId};
}

THolder<TApiSkillResponse> ConstructFakeData(const TSlot& slot, TContext& ctx,
                                                            TErrorBlock::TResult* result,
                                                            TMaybe<NSc::TValue>& nerInfo,
                                                            bool* isConsole,
                                                            NHttpFetcher::TRequest* fakeRequest)
{
    const TSlot* const slotSkillTest = ctx.GetSlot("skill_description");
    if (IsSlotEmpty(slotSkillTest)) {
        auto& resolver = ISkillResolver::GlobalResolver();
        const TStringBuf skillId = slot.Value.GetString();

        TMaybe<TNerRequester> nerRequester;

        ISkillResolver::TSkillResponsePtr skillResponse = resolver.ResolveSkillId(
            ctx, skillId, ctx.GetConfig(), result
        );

        if (!skillResponse) {
            // create an invalid object for the back compatibility
            skillResponse = MakeHolder<TApiSkillResponse>();
        }

        if (skillResponse->Scheme().Result().UseNLU()) {
            nerRequester = NerRequestWithFakeRequest(ctx, fakeRequest);
        }

        OnNerResponse(std::move(nerRequester), nerInfo);

        return skillResponse;
    }

    // FIXME make separate ISkillResolver; check if test is working

    // TODO make request only in case useNLU is turned off
    OnNerResponse(NerRequestWithFakeRequest(ctx, fakeRequest), nerInfo);

    NSc::TValue json;
    *isConsole = true;
    json["result"] = slotSkillTest->Value;
    json["onAir"].SetBool(true); // always enabled when it is from api console for checking

    return MakeHolder<TApiSkillResponse>(std::move(json));
}

////////// TSkillDescription ///////////////////////////////////////////////////////////////////////////////////////////
TSkillDescription::TStylesMap TSkillDescription::Styles;

TSkillDescription::TSkillDescription(TContext& ctx)
{
    const TSlot* const slotSkillTest = ctx.GetSlot("skill_description");
    if (IsSlotEmpty(slotSkillTest)) {
        IsConsole = false;
    } else {
        IsConsole = true;
    }
}

void TSkillDescription::Init(const TSlot& slot, TContext& ctx) {
    Data = ConstructData(slot, ctx, &ResultValue, NerInfo, IsConsole);
    SkillNameForMetrics = BuildSkillNameForMetrics(Data->Scheme().Result().Title());
    DeveloperMode = IsSkillInDeveloperMode(ctx, Data->Scheme().Result().Id());
    if (!ResultValue && !Data->Scheme().Result().OnAir()) {
        ResultValue.ConstructInPlace(TError::EType::SKILLDISABLED, TStringBuf("skill is not enabled"));
    }

    Labels.Add("skill", Scheme().Id());
    Labels.Add("type", Scheme().MonitoringType());
    Labels.Add("source", GetSkillRequestSource(ctx, IsConsole));
}

TSkillDescription::TSkillDescription(const TSlot& slot, TContext& ctx, NHttpFetcher::TRequest* fakeRequest)
    : IsConsole(false)
    , Data(ConstructFakeData(slot, ctx, &ResultValue, NerInfo, &IsConsole, fakeRequest))
    , SkillNameForMetrics(BuildSkillNameForMetrics(Data->Scheme().Result().Title()))
    , DeveloperMode(IsSkillInDeveloperMode(ctx, Data->Scheme().Result().Id()))
{
    if (!ResultValue && !Data->Scheme().Result().OnAir()) {
        ResultValue.ConstructInPlace(TError::EType::SKILLDISABLED, TStringBuf("skill is not enabled"));
    }

    Labels.Add("skill", Scheme().Id());
    Labels.Add("type", Scheme().MonitoringType());
    Labels.Add("source", GetSkillRequestSource(ctx, IsConsole));
}

void TSkillDescription::WriteInfo(TContext* ctx) const {
    Y_ASSERT(ctx);

    const auto& result = Data->Scheme().Result();

    NSc::TValue infoJson;
    infoJson["name"].SetString(result.Title());
    infoJson["internal"].SetBool(result.IsInternal());
    infoJson["zora"].SetBool(result.UseZora());
    if (result.Voice() != TStringBuf("alice")) {
        infoJson["voice"].SetString(result.Voice());
    }
    if (!result.BotGuid()->empty()) {
        infoJson["bot_guid"].SetString(result.BotGuid());

        if (ctx->MetaClientInfo().IsDesktop()) {
            TCgiParameters cgi;
            cgi.InsertUnescaped("socketUrl", "wss://chat.ws.yandex.ru/chat/");
            cgi.InsertUnescaped("orgId", result.Id());
            cgi.InsertUnescaped("orgName", result.Title());
            cgi.InsertUnescaped("parentOrigin", "https://yandex.ru");
            infoJson["open_bot_url"].SetString(TStringBuilder() << "https://yandex.ru/chat?" << cgi.Print());
        }
    }
    infoJson[JSNAME_DEVELOPERMODE].SetBool(DeveloperMode);
    ctx->CreateSlot(SLOTNAME_SKILLINFO, "skill_info", true, std::move(infoJson));
}

void TSkillDescription::WriteUpdateDialogInfo(TContext* ctx) const {
    Y_ASSERT(ctx);

    if (!ctx->ShouldAddSkillStyle() || !IsTabSkill()) {
        return;
    }

    const auto& result = Data->Scheme().Result();

    TSchemeHolder<NBASSExternalSkill::TChatInfo<TSchemeTraits>> scheme;
    scheme->Title() = result.Title();
    scheme->ImageUrl() = CreateImageUrl(*ctx, result.Logo().AvatarId(), IMAGE_TYPE_MOBILE_LOGO, AVATAR_NAMESPACE_SKILL_LOGO);
    scheme->Url() = result.StoreUrl();
    scheme->Style().Assign(Style());
    scheme->DarkStyle().Assign(DarkStyle());

    if (OpenInNewTab(*ctx)) {
        TSchemeHolder<NBASSExternalSkill::TChatInfo<TSchemeTraits>::TMenuItem> menuItemScheme;

        menuItemScheme->Url() = result.StoreUrl();
        menuItemScheme->Title() = TStringBuf("Описание навыка");
        scheme->MenuItems().Add() = menuItemScheme.Scheme();
    }

    scheme->ListeningIsPossible() = true;

    ctx->AddCommand<TExternalSkillDescriptionUpdateDialogInfoDirective>(COMMAND_CHAT_INFO, std::move(scheme.Value()));
}

TSkillDescription::TStyleScheme TSkillDescription::FindStyleByName(const TStringBuf styleName) const {
    const auto* style = Styles.FindPtr(styleName);
    if (!style) {
        ythrow yexception() << "style '" << styleName << "' doesn't not exist in config";
    }

    return style->Scheme();
}

TSkillDescription::TStyleScheme TSkillDescription::Style() const {
    const TStringBuf look{Data->Scheme().Result().Look()};
    return FindStyleByName(look);
}

TSkillDescription::TStyleScheme TSkillDescription::DarkStyle() const {
    TStringBuilder look;
    look << Data->Scheme().Result().Look() << "_dark";
    return FindStyleByName(look);
}

bool TSkillDescription::IsTabSkill() const {
    return Data->Scheme().Result().OpenInNewTab();
}

bool TSkillDescription::OpenInNewTab(const TContext& ctx) const {
    return IsTabSkill() && ctx.ClientFeatures().SupportsMultiTabs();
}

void TSkillDescription::WriteDebugSlot(TContext& ctx, const NSc::TValue& request, TStringBuf response) const {
    if (!IsConsole) {
        return;
    }

    NSc::TValue data;

    data["request"] = request;
    if (IsUtf(response)) {
        data["response_raw"].SetString(response);
    } else {
        data["response_raw"].SetNull();
    }

    ctx.CreateSlot("skill_debug", "json", true, std::move(data));
}

// static
void TSkillDescription::InitStylesFromConfig(const TConfig& config) {
    using TStyleScheme = NBASSExternalSkill::TStyle<TSchemeTraits>;
    THashMap<TString, TSchemeHolder<TStyleScheme>> styles;

    TStringBuilder errmsg;
    auto errCb = [&errmsg](TStringBuf path, TStringBuf msg) {
        if (errmsg) {
            errmsg << "; ";
        }
        errmsg << path << " = " << msg;
    };

    for (const auto& kv : config->SkillStyles()) {
        TSchemeHolder<TStyleScheme> scheme(*kv.Value().GetRawValue());
        if (!scheme->Validate("", false, errCb)) {
            LOG(ERR) << "Invalid style in for external skills (skip it) in config: " << errmsg << Endl;
            continue;
        }

        styles.emplace(kv.Key(), std::move(scheme));
    }

    Styles.swap(styles);
}

// static
TString TSkillDescription::CreateImageUrl(const TContext& ctx, TStringBuf imageId, TStringBuf imageType, TStringBuf ns) {
    TStringBuilder url;
    url << TStringBuf("https://")
        << ctx.GetConfig()->Vins()->ExternalSkillsApi()->AvatarsHost();

    addIfNotLast(url, '/');

    url << "get-" <<  ns << '/'
        << imageId << '/'
        << imageType;

    if (imageType == IMAGE_TYPE_BIG || imageType == IMAGE_TYPE_SMALL || imageType == IMAGE_TYPE_MOBILE_LOGO || imageType == IMAGE_TYPE_LOGO_FG_IMG) {
        url << ctx.MatchScreenScaleFactor(ALLOWED_SCREEN_SCALE_FACTORS_DEFAULT);
    } else if (imageType == IMAGE_TYPE_LOGO_BG_IMG) {
        url << ctx.MatchScreenScaleFactor(ALLOWED_SCREEN_SCALE_FACTORS_LARGE_IMAGES);
    }

    return url;
}

TErrorBlock::TResult TSkillDescription::RequestSkill(
    TContext& ctx,
    TSession& session,
    std::unique_ptr<ISkillParser>* parser,
    TSkillDiagnosticInfo& diagnosticInfo) const
{
    Y_ASSERT(parser);

    ESkillRequestType requestType = ESkillRequestType::Default;
    if (ctx.HasInputAction()) {
        if (ctx.InputAction()->Name == ACTION_SKILL_PURCHASE_COMPLETE) {
            requestType = ESkillRequestType::SkillsPurchaseComplete;
        } else if (ctx.InputAction()->Name == ACTION_SKILL_ACCOUNT_LINKING_COMPLETE) {
            requestType = ESkillRequestType::AccountLinkingComplete;
        }
    }

    const auto& skillResult = Data->Scheme().Result();

    if (requestType != ESkillRequestType::Default) {
        if (auto initialDeviceId = ctx.InputAction()->Data["initial_device_id"];
            !initialDeviceId.IsNull() && !initialDeviceId.GetString().empty())
        {
            TString uid;
            TPersonalDataHelper(ctx).GetUid(uid);

            NHttpFetcher::TRequestPtr request = ctx.GetSources().XivaProvider().Request();

            request->AddCgiParam("user", uid);
            request->AddCgiParam("event", "server_action");
            request->AddCgiParam("ttl", "360");

            NSc::TValue payload;
            payload["type"] = "server_action";
            payload["name"] = "bass_action_with_update_form_at_first";
            payload["payload"]["name"] = ctx.InputAction()->Name;
            payload["payload"]["data"]["form"]["set_new_form"] = true;
            payload["payload"]["data"]["form"]["name"].SetString("personal_assistant.scenarios.external_skill__continue");
            auto& node = payload["payload"]["data"]["form"]["slots"].SetArray().Push();
            node["name"].SetString("skill_id");
            node["optional"] = true;
            node["type"].SetString("string");
            node["value"].SetString(skillResult.Id());

            NSc::TValue body;
            body["payload"].SetString(payload.ToJson());
            body["subscriptions"].SetArray().Push()["session"].Push().SetString(initialDeviceId.GetString());

            request->SetBody(body.ToJson(), TStringBuf("POST"));
            request->SetContentType(TStringBuf("application/json"));
            request->AddHeader("Authorization", TStringBuilder{} << "Xiva " << ctx.GlobalCtx().Config().PushHandler().XivaProvider().Token());

            auto response = request->Fetch()->Wait();
            if (response->IsHttpOk()) {
                LOG(INFO) << "Skills xiva push sent" << Endl;
                LOG(DEBUG) << response->Data << Endl;
                Y_STATS_INC_COUNTER("bass_send_xiva_skill_push_success");
            } else {
                LOG(ERR) << "Failed to send xiva skill push, error: " << response->GetErrorText() << Endl;
                Y_STATS_INC_COUNTER("bass_failed_to_send_xiva_skill_push_error");
            }

            if (requestType == ESkillRequestType::AccountLinkingComplete) {
                ctx.AddAttention(SKILL_ACCOUNT_LINKING_SUCCESS_DELEGATE);
                ctx.AddStopListeningBlock();
            } else if (requestType == ESkillRequestType::SkillsPurchaseComplete) {
                ctx.AddAttention(QUASAR_BILLING_SKILLS_SUCCESS_DELEGATE);
                ctx.AddStopListeningBlock();
            }
            return TErrorBlock::Ok;
        }
    }

    if (skillResult.BackendSettings()->GetRawValue()->DictEmpty()) {
        return TErrorBlock("Unable to create request to skill (backend settings is empty)", ERRTYPE_BADURL, "skill_url");
    }

    NSc::TValue requestJson;
    if (TErrorBlock::TResult err = ISkillParser::TCurrentSkillParser::PrepareRequest(ctx, *this, session, requestType, &requestJson)) {
        return err;
    }

    TMaybe<TString> bearerToken;
    if (TErrorBlock::TResult err = GetSocialToken(ctx, skillResult, &bearerToken)) {
        return err;
    }
    if (bearerToken) {
        if (skillResult.Id() == "7bf49045-c077-430a-938c-10858843dfed") {
            // Temporary hardcode until Golfstrim changes their skill
            bearerToken = "OAuth " + *bearerToken;
        } else {
            bearerToken = "Bearer " + *bearerToken;
        }
    }

    const auto& vinsConfig = ctx.GetConfig().Vins();
    const auto& skillsConfig = vinsConfig.ExternalSkillsApi();
    const TString& skillNameNorm = GetSkillNameForMetrics();
    const TDuration skillTimeout = skillsConfig.SkillTimeout();

    NHttpFetcher::TRequestOptions options;
    options.Timeout = skillTimeout;
    options.MaxAttempts = 1;
    options.EnableFastReconnect = true;


    NHttpFetcher::TRequestPtr r;
    TString functionId(TString{*skillResult.BackendSettings().FunctionId()});
    if (!functionId.empty()) {
        r = PrepareYaFunctionsRequest(ctx, functionId, skillTimeout);
    } else {
        try {
            r = PrepareZoraRequest(ctx, skillResult, options);
        } catch (const yexception& e) {
            LOG(ERR) << e.what() << Endl;
            return TErrorBlock("Unable to create request to skill", ERRTYPE_BADURL, "skill_url");
        }
    }

    r->AddHeader("X-Request-Id", ctx.Meta().RequestId());

    r->SetBody(requestJson.ToJson(), TStringBuf("POST"));
    r->SetContentType(TStringBuf("application/json"));
    if (bearerToken) {
        r->AddHeader(TStringBuf("Authorization"), *bearerToken);
    }
    LOG(DEBUG) << "skill request: " << TLogging::AsCurl(r->Url(), "") << Endl;

    NHttpFetcher::TResponse::TRef resp;
    {
        Y_STATS_SCOPE_HISTOGRAM("bass_skill_request_time");
        Y_STATS_SCOPE_HISTOGRAM(TStringBuilder() << "bass_skill_" << skillNameNorm << "_request_time");
        resp = r->Fetch()->WaitFor(skillTimeout);
    }
    for (const auto& t : resp->Headers) {
        if (t.Name() == "X-Yandex-Orig-Http-Code") {
            Y_STATS_INC_COUNTER(TStringBuilder{} << "bass_skill_zora_http_code_" << t.Value());
            break;
        }
    }

    LOG(DEBUG) << "skill response time: " << resp->Duration << Endl;
    WriteDebugSlot(ctx, requestJson, resp->Data);

    UpdateSkillCounters(ToString(Scheme().Id()), skillNameNorm, ToString(Scheme().MonitoringType()), resp, ctx.GlobalCtx());

    diagnosticInfo.SetSkillId(TString{*skillResult.Id()});
    diagnosticInfo.SetClientId(TString{requestJson["meta"]["client_id"].GetString()});
    diagnosticInfo.SetRequestId(TLogging::ReqInfo.Get().ReqId());
    diagnosticInfo.SetSessionId(TString{requestJson["session"]["session_id"].GetString()});
    diagnosticInfo.SetUserId(TString{requestJson["session"]["user_id"].GetString()});
    diagnosticInfo.SetSource(GetSkillRequestSource(ctx, IsConsole));
    diagnosticInfo.SetErrorType("");
    diagnosticInfo.SetErrorDetail(resp->GetErrorText());
    diagnosticInfo.SetTimestampMcr(Now().MicroSeconds());
    diagnosticInfo.SetZoraResponseTimeMcr(resp->Duration.MicroSeconds());

    LOG(DEBUG) << "Skill response: " << resp->Data << Endl;

    auto err = ProcessSkillResponse(ctx, session, parser, resp, diagnosticInfo, r->Url(), skillNameNorm, skillResult, requestJson);

    if (!err || requestType == ESkillRequestType::Default) {
        return err;
    }

    if (requestType == ESkillRequestType::AccountLinkingComplete) {
        ctx.AddAttention(SKILL_ACCOUNT_LINKING_SUCCESS);
    } else if (requestType == ESkillRequestType::SkillsPurchaseComplete) {
        ctx.AddAttention(QUASAR_BILLING_SKILLS_SUCCESS);
    }
    return TErrorBlock::Ok;
}

TErrorBlock::TResult TSkillDescription::ProcessSkillResponse(
    TContext& ctx,
    TSession& session,
    std::unique_ptr<ISkillParser>* parser,
    NHttpFetcher::TResponse::TRef resp,
    TSkillDiagnosticInfo& diagnosticInfo,
    const TString& url,
    const TString& skillNameNorm,
    const TApiSkillResponseScheme::TResultConst& skillResult,
    const NSc::TValue& requestJson) const
{
    if (!resp->IsHttpOk()) {
        TErrorBlock errBlock(TError::EType::SKILLSERROR, TStringBuilder() << "Fetch error '" << url << "': " << resp->GetErrorText());
        LogWebhookError(resp);
        Y_STATS_INC_COUNTER_IF(IsZoraError(resp), "bass_skill_zora_error");
        switch (resp->Result) {
            case NHttpFetcher::TResponse::EResult::Timeout:
            {
                errBlock.AddProblem(ERRTYPE_TIMEOUT, "skill_url");
                WriteCounters(ctx, REQUEST_ERROR_TIMEOUT, skillNameNorm);
                diagnosticInfo.SetErrorType(REQUEST_ERROR_TIMEOUT);
                Y_STATS_INC_COUNTER("bass_skill_zora_timeout");
            }
                break;
            case NHttpFetcher::TResponse::EResult::EmptyResponse:
            case NHttpFetcher::TResponse::EResult::ParsingError:
            case NHttpFetcher::TResponse::EResult::DataError:
            {
                errBlock.AddProblem(ERRTYPE_HTTP_PARSE, "skill_url");
                WriteCounters(ctx, REQUEST_ERROR_PARSE, skillNameNorm);
                diagnosticInfo.SetErrorType(REQUEST_ERROR_PARSE);
            }
                break;
            case NHttpFetcher::TResponse::EResult::Ok:
            case NHttpFetcher::TResponse::EResult::HttpError:
            {
                if (ZoraReturnedTimeout(resp)) {
                    errBlock.AddProblem(ERRTYPE_TIMEOUT, "skill_url");
                    WriteCounters(ctx, REQUEST_ERROR_TIMEOUT, skillNameNorm);
                    diagnosticInfo.SetErrorType(REQUEST_ERROR_TIMEOUT);
                } else {
                    WriteCounters(ctx, REQUEST_ERROR_HTTP, skillNameNorm);
                    diagnosticInfo.SetErrorType(REQUEST_ERROR_HTTP);
                    if (resp->Code == HttpCodes::HTTP_BAD_GATEWAY && IsSslCertError(resp->GetErrorText())) {
                        WriteCounters(ctx, REQUEST_ERROR_HTTP_SSL_CERT, skillNameNorm);
                        diagnosticInfo.SetErrorType(REQUEST_ERROR_HTTP_SSL_CERT);
                    }
                    NSc::TValue data;
                    data["code"].SetIntNumber(resp->Code);
                    errBlock.AddProblem(ERRTYPE_FETCH, "skill_url", std::move(data));
                }
            }
                break;
            case NHttpFetcher::TResponse::EResult::NetworkResolutionError:
            {
                // TODO (@vi002): implement this
                errBlock.AddProblem(ERRTYPE_NAME_RESOLUTION_ERROR,  "skill_url");
                WriteCounters(ctx, REQUEST_NAME_RESOLUTION_ERROR, skillNameNorm);
                diagnosticInfo.SetErrorType(REQUEST_NAME_RESOLUTION_ERROR);
            }
                break;
        }
        return errBlock;
    }

    NSc::TValue skillRespJson;
    try {
        skillRespJson = NSc::TValue::FromJsonThrow(resp->Data, NSc::TValue::TJsonOpts(NSc::TValue::TJsonOpts::EJsonOpts::JO_PARSER_STRICT_JSON));
        TVector<TString> includedMajorFields;
        for (const auto& field : SKILL_RESPONSE_MAJOR_FIELDS) {
            if (skillRespJson.Has(field)) {
                includedMajorFields.push_back(field);
            }
        }
        if (includedMajorFields.size() > 0 && skillRespJson.Has(SKILL_RESPONSE_MINOR_FIELD)) {
            skillRespJson.Delete(SKILL_RESPONSE_MINOR_FIELD);
        }
        if (includedMajorFields.size() > 1) {
            LOG(ERR) << "skill response json: " << skillRespJson << Endl;
            WriteCounters(ctx, REQUEST_ERROR_PARSE, skillNameNorm);
            diagnosticInfo.SetErrorType(REQUEST_ERROR_PARSE);
            return TErrorBlock("unsupported skill response: too many fields", ERRTYPE_MANY, JoinSeq(" ", includedMajorFields));
        }

        *parser = ISkillParser::CreateParser(session, skillRespJson, *this);
        if (!*parser) {
            LOG(ERR) << "skill response json: " << skillRespJson << Endl;
            WriteCounters(ctx, REQUEST_ERROR_PARSE, skillNameNorm);
            diagnosticInfo.SetErrorType(REQUEST_ERROR_PARSE);
            return TErrorBlock("unsupported skill response version", ERRTYPE_VERSION, "/version");
        }
    }
    catch (const NSc::TSchemeParseException& e) {
        LOG(DEBUG) << "skill response json error: JSON: " << resp->Data << Endl;
        NSc::TValue jsonData;
        jsonData["offset"].SetIntNumber(e.Offset);
        jsonData["body"].SetString(resp->Data);
        if (Data->Scheme().Result().IsInternal()) {
            jsonData["reason"].SetString(e.Reason);
        }
        WriteCounters(ctx, REQUEST_ERROR_PARSE, skillNameNorm);
        diagnosticInfo.SetErrorType(REQUEST_ERROR_PARSE);
        return TErrorBlock(TError::EType::SKILLSERROR, e.what()).AddProblem(ERRTYPE_BADJSON, "skill_url", jsonData);
    }
    catch (...) {
        TString error = CurrentExceptionMessage();
        diagnosticInfo.SetErrorType(error);
        return TErrorBlock(error, ERRTYPE_UNKNOWN, "skill_url");
    }

    if (skillRespJson.Has("start_purchase")) {
        if (!DeviceSupportsBilling(ctx)) {
            ctx.AddAttention(QUASAR_BILLING_SKILLS_DEVICE_IS_NOT_SUPPORTED);
            return TErrorBlock::Ok;
        }
        if (Find(skillResult.FeatureFlags(), "billing") != skillResult.FeatureFlags().end() ||
            Find(SkillsWhiteList, TString{*skillResult.Id()}) != SkillsWhiteList.end())
        {
            TString uid;
            TPersonalDataHelper(ctx).GetUid(uid);
            RequestQuasarBillingSkills(ctx, skillResult, requestJson, skillRespJson, uid);
        } else {
            return TErrorBlock("either skill or user is not allowed to use billing", ERRTYPE_VERSION, "/version");
        }
    }

    if (skillRespJson.Has("start_account_linking")) {
        if (!DeviceSupportsAccountLinking(ctx)) {
            ctx.AddAttention(SKILL_ACCOUNT_LINKING_DEVICE_IS_NOT_SUPPORTED);
            return TErrorBlock::Ok;
        }
        TString uid;
        TPersonalDataHelper(ctx).GetUid(uid);
        StartAccountLinking(ctx, skillResult, uid);
    }
    return TErrorBlock::Ok;
}

void TSkillDescription::WriteCounters(TContext& ctx, const TString& counterName, const TString& skillNameNorm) const {
    Y_STATS_INC_COUNTER(TStringBuilder() << "bass_skill_" << counterName);
    Y_STATS_INC_COUNTER(TStringBuilder() << "bass_skill_" << skillNameNorm << "_" << counterName);

    Sensors().Inc(SignalLabels(), counterName);
    SkillSensors(ctx).Inc(SignalLabels(), counterName);
}

TSession::TSession(TContext& ctx)
    : SeqNumber(0)
{
    const TSlot* slot = ctx.GetSlot("session");
    if (!IsSlotEmpty(slot)) {
        Guid = slot->Value["id"].GetString();
        SeqNumber = slot->Value["seq"].GetIntNumber() + 1ULL;
    }

    if (!Guid) {
        Guid = CreateGuidAsString();
    }
}

void TSession::UpdateContext(TContext& ctx) const {
    NSc::TValue session;
    session["id"].SetString(Guid);
    session["seq"].SetIntNumber(SeqNumber);
    ctx.CreateSlot("session", "session", false, std::move(session));
}

// static
std::unique_ptr<ISkillParser> ISkillParser::CreateParser(const TSession& session, const NSc::TValue& response,
                                                         const TSkillDescription& skill) {
    TMaybe<TSemVer> version = TSemVer::FromString(response["version"].GetString());
    if (!version) {
        return {};
    }

    if (1 == version->Major) {
        return std::make_unique<TSkillParserVersion1x>(session, skill, response);
    }

    return {};
}

} // namespace NExternalSkill
} // namespace NBASS
