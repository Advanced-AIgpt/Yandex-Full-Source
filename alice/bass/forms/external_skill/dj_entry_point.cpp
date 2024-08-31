#include "dj_entry_point.h"

#include "fwd.h"
#include "skill.h"

#include <alice/bass/forms/common/data_sync_api.h>
#include <alice/bass/forms/external_skill_recommendation/enums.h>

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <dj/services/alisa_skills/server/proto/client/request.pb.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/string/cast.h>


namespace NBASS::NExternalSkill {
namespace {

constexpr TStringBuf LOOK_INTERNAL_OLD = "internalOld_";

bool Validate(const TServiceResponse& response) {
    TStringBuilder err;
    auto validateCb = [&err](TStringBuf key, TStringBuf errmsg) {
        err << key << ": " << errmsg << "; ";
    };
    if (!response->Validate("", false, validateCb)) {
        LOG(ERR) << "Not valid response from service: " << err << Endl;
        return false;
    }

    return true;
}

} // namespace

TString ConstructLogoUrl(const TStringBuf look, const TStringBuf prefix, const TStringBuf avatarId, const TContext& ctx) {
    if (look != LOOK_INTERNAL_OLD) {
        return TSkillDescription::CreateImageUrl(ctx, avatarId, IMAGE_TYPE_MOBILE_LOGO, AVATAR_NAMESPACE_SKILL_LOGO);
    }
    const TAvatar* avatar = ctx.Avatar(prefix, avatarId);
    if (avatar) {
        return avatar->Https;
    }

    LOG(ERR) << "Not found avatar. Prefix: " << prefix << " AvatarId: " << avatarId << Endl;
    return "error";
}

NHttpFetcher::TRequestPtr PrepareRequest(TContext& ctx, EServiceRequestCard cardName, NHttpFetcher::IMultiRequest::TRef multiRequest) {
    TCgiParameters cgi;
    NDJ::NAS::TServiceRequest protoReq;

    protoReq.SetCardName(ToString(cardName));
    cgi.InsertUnescaped(TStringBuf("card_name"), ToString(cardName)); //Also sending card_name as cgi for logging purposes
    cgi.InsertUnescaped(TStringBuf("msid"), ctx.Meta().RequestId().Get().Data());
    cgi.InsertUnescaped(TStringBuf("client"), TStringBuf("bass"));

    protoReq.MutableUserData()->SetUuid(ctx.Meta().UUID().Get().Data());

    if (ctx.Meta().HasDeviceId()) {
        protoReq.MutableUserData()->SetDeviceId(ctx.Meta().DeviceId().Get().Data());
    }
    TString puid;
    if (NBASS::TPersonalDataHelper(ctx).GetUid(puid)) {
        protoReq.MutableUserData()->SetPuid(puid);
    }
    const auto src = ctx.Meta().Experiments();
    TSet<TStringBuf> expFlags;
    if (src.IsDict()) {
        for (const auto& kv : src->GetDict()) {
            if (!kv.second.IsNull()) {
                expFlags.insert(kv.first);
            }
        }
    } else if (src.IsArray()) {
        for (const NSc::TValue& e : src->GetArray()) {
            expFlags.insert(e.GetString());
        }
    }

    for (const auto& flag : expFlags) {
        protoReq.AddAliceExperiments(flag.Data());
    }
    const auto& client = ctx.ClientFeatures();
    if (client.SupportsTimers()) {
        protoReq.AddPlatformFeatures(ToString(ESkillRequirement::Timers));
    }
    if (client.SupportsAlarms()) {
        protoReq.AddPlatformFeatures(ToString(ESkillRequirement::Alarms));
    }
    if (client.SupportsImageRecognizer()) {
        protoReq.AddPlatformFeatures(ToString(ESkillRequirement::ImageRecognizer));
    }
    if (ctx.ClientFeatures().SupportsMusicRecognizer()) {
        protoReq.AddPlatformFeatures(ToString(ESkillRequirement::MusicRecognizer));
    }
    if (ctx.ClientFeatures().SupportsDivCards()) {
        protoReq.AddPlatformFeatures(ToString(ESkillRequirement::DivCards));
    }
    if (ctx.ClientFeatures().SupportsOpenWhocalls()) {
        protoReq.AddPlatformFeatures(ToString(ESkillRequirement::WhoCalls));
    }
    if (ctx.ClientFeatures().SupportsOpenKeyboard()) {
        protoReq.AddPlatformFeatures(ToString(ESkillRequirement::Keyboard));
    }
    if (ctx.ClientFeatures().SupportsNavigator()) {
        protoReq.AddPlatformFeatures(ToString(ESkillRequirement::Navigator));
    }
    TString clientIdEscaped = ctx.MetaClientInfo().ClientId;
    Quote(clientIdEscaped);
    protoReq.MutableUserData()->SetClientId(clientIdEscaped);
    TString timezoneEscaped = ctx.UserTimeZone();
    Quote(timezoneEscaped);
    protoReq.SetTimetz(TStringBuilder() << ctx.Meta().Epoch() << '@' << timezoneEscaped);
    protoReq.SetLanguage(ctx.Meta().Lang().Get().Data());
    if (ctx.Meta().HasScreenScaleFactor()) {
        protoReq.SetScreenScaleFactor(ctx.Meta().ScreenScaleFactor());
    }
    protoReq.SetRngSeed(MultiHash(ctx.GetRngSeed()));

    TString body;
    NProtobufJson::TProto2JsonConfig cfg;
    cfg.FieldNameMode = NProtobufJson::TProto2JsonConfig::FieldNameSnakeCase;
    NProtobufJson::Proto2Json(protoReq, body, cfg);
    LOG(DEBUG) << "Sending AliceSkillService request. Cgi: " << cgi.Print() << "; Post: " << body << Endl;

    const auto& recommender = ctx.GetSources().ExternalSkillsRecommender();
    auto request = multiRequest ? recommender.AttachRequest(multiRequest) : recommender.Request();
    request->SetContentType("Content-Type: application/json");
    request->SetBody(body);
    request->AddCgiParams(cgi);
    request->SetMethod("POST");
    return request;
}

bool ParseResponse(TContext& ctx, NHttpFetcher::TResponse::TConstRef response, TServiceResponse& answer) {
    if (response->IsError()) {
        LOG(ERR) << "Fetching from recommendation service error: " << response->GetErrorText() << Endl;
        Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), ToString(EStatsIncConuter::ServiceError));

        return false;
    }

    answer = TServiceResponse(NSc::TValue::FromJson(response->Data));
    return Validate(answer);
}

bool TryGetRecommendationsFromService(TContext& ctx, EServiceRequestCard cardName, TServiceResponse& answer) {
    const auto& request = PrepareRequest(ctx, cardName);
    auto response = request->Fetch()->Wait();
    return ParseResponse(ctx, response, answer);
}

} // namespace NBASS::NExternalSkill
