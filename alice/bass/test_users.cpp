#include "test_users.h"

#include "test_users_details.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/reminders/reminder.h>
#include <alice/bass/forms/reminders/todo.h>
#include <alice/bass/forms/special_location.h>
#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/utility.h>

namespace NBASS {
namespace {

constexpr ui64 DEFAULT_USER_TIMEOUT = TDuration::Seconds(10).Seconds();

const THashMap<TStringBuf, std::function<bool(TContext&)>> CLEANUP_HANDLERS {
    {
        "oauth",
        [](TContext& ctx) {
            ctx.DeleteSavedAddress(TSpecialLocation(TSpecialLocation::EType::HOME));
            ctx.DeleteSavedAddress(TSpecialLocation(TSpecialLocation::EType::WORK));
            return true;
        }
    },
    {
        "reminder",
        NReminders::ReminderCleanupForTestUser
    },
    {
        "todo",
        NReminders::TodoCleanupForTestUser
    },
    {
        "kolonkish",
        TPersonalDataHelper::CleanupTestUserKolonkish
    },
};

class TGetUserResultVisitor {
public:
    TGetUserResultVisitor(TGlobalContextPtr globalCtx, NTestUsersDetails::TUserManager& um, ui64 now, NSc::TValue& out)
        : GlobalCtx_(globalCtx)
        , UserManager_(um)
        , Now_(now)
        , Out_(out)
    {
    }

    HttpCodes operator()(const NTestUsersDetails::TUserManager::TErrorResult& res) {
        Out_["error"] = res.Json;
        return res.HttpCode;
    }

    HttpCodes operator()(const NTestUsersDetails::TUserManager::TSuccessResult& res) {
        NSc::TValue requestJson;
        NSc::TValue& metaJson = requestJson["meta"];
        metaJson["uuid"] = res["uuid"];
        metaJson["client_ip"] = res["client_ip"];
        metaJson["tz"].SetString("Europe/Moscow");
        metaJson["epoch"] = Now_;

        TStringBuilder authHeader;
        if (TStringBuf token = res["token"].GetString()) {
            authHeader << TStringBuf("OAuth ") << token;
        }

        TContext::TPtr ctx;
        TContext::TInitializer initData{GlobalCtx_, "dummy_reqid", authHeader, /* appInfoHeader= */ "",
                                        /* fakeTimeHeader= */ "", /* userHeaderTicket= */ {}, {} /* SpeechKitEvent */};
        if (TResultValue rval = TContext::FromJson(requestJson, initData, &ctx)) {
            return (*this)(NTestUsersDetails::TUserManager::TErrorResult{HTTP_BAD_REQUEST, TStringBuilder() << "Unable to create context: " << rval->Msg});
        }

        const NSc::TValue::TArray& tags = res["tags"].GetArray();
        for (const auto& cleanupHandler : CLEANUP_HANDLERS) {
            if (FindIndex(tags, cleanupHandler.first) != NPOS) {
                if (!cleanupHandler.second(*ctx)) {
                    UserManager_.ReleaseUser(res["login"].GetString());

                    return (*this)(NTestUsersDetails::TUserManager::TErrorResult{HTTP_BAD_REQUEST, "cleanup", cleanupHandler.first, ""});
                }
            }
        }

        Out_["result"] = res;

        return HTTP_OK;
    }

private:
    TGlobalContextPtr GlobalCtx_;
    NTestUsersDetails::TUserManager& UserManager_;
    const ui64 Now_;
    NSc::TValue& Out_;
};

class TReleaseUserResultVisitor {
public:
    explicit TReleaseUserResultVisitor(NSc::TValue& out)
        : Out(out)
    {
    }

    HttpCodes operator()(const NTestUsersDetails::TUserManager::TErrorResult& res) {
        Out["error"] = res.Json;
        return res.HttpCode;
    }

    HttpCodes operator()(const NTestUsersDetails::TUserManager::TSuccessResult& /*res*/) {
        Out["result"].SetString("ok");
        return HTTP_OK;
    }

private:
    NSc::TValue& Out;
};

} // namespace

TTestUsersRequestHandler::TTestUsersRequestHandler(TGlobalContextPtr globalCtx)
    : GlobalCtx{globalCtx}
    , UserManager{MakeHolder<NTestUsersDetails::TUserManager>(globalCtx->Config()->TestUsersYDb())}
{
}

const TString& TTestUsersRequestHandler::GetReqIdClass() const {
    static const TString reqidClass{"test"};
    return reqidClass;
}

HttpCodes TTestUsersRequestHandler::DoJsonReply(TGlobalContextPtr /*globalCtx*/, const NSc::TValue& in,
                                                const TParsedHttpFull& /* http */, const THttpHeaders& /* headers */,
                                                NSc::TValue* out)
{
    TStringBuf method = in["method"].GetString();
    const NSc::TValue& args = in["args"];
    HttpCodes result = HTTP_BAD_REQUEST;
    if (method == TStringBuf("GetUser")) {
        result = GetUser(args, out);
    } else if (method == TStringBuf("FreeUser")) {
        result = ReleaseUser(args, out);
    }

    LOG(DEBUG) << "response " << HttpCodeStr(result) << ": " << out->ToJson() << Endl;
    return result;
}

HttpCodes TTestUsersRequestHandler::GetUser(const NSc::TValue& req, NSc::TValue* out) {
    const ui64 now = TInstant::Now().Seconds();
    const ui64 releaseAt = now + Max(static_cast<ui64>(req["timeout"]), DEFAULT_USER_TIMEOUT);

    return std::visit(TGetUserResultVisitor(GlobalCtx, *UserManager, now, *out), UserManager->GetUser(req["tags"], now, releaseAt));
}

HttpCodes TTestUsersRequestHandler::ReleaseUser(const NSc::TValue& user, NSc::TValue* out) {
    TStringBuf login{user["login"].GetString()};
    if (!login) {
        return TReleaseUserResultVisitor{*out}(
            NTestUsersDetails::TUserManager::TErrorResult{HTTP_BAD_REQUEST, "request", "login is a required field", "post_json"}
        );
    }

    return std::visit(TReleaseUserResultVisitor{*out}, UserManager->ReleaseUser(login));
}

// static
void TTestUsersRequestHandler::RegisterHttpHandlers(THttpHandlersMap* handlers, TGlobalContextPtr globalCtx) {
    static IHttpRequestHandler::TPtr handler = new TTestUsersRequestHandler(globalCtx);
    handlers->Add(TStringBuf("/test_user"), []() {
        return handler;
    });
}

} // namespace NBASS
