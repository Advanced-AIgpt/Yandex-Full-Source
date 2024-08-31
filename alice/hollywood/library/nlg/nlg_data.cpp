#include "nlg_data.h"

#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood {


TNlgData::TNlgData(TRTLogger& logger, const TScenarioBaseRequestWrapper& request)
    : Context(NJson::TJsonValue{NJson::JSON_MAP})
    , ReqInfo(NJson::TJsonValue{NJson::JSON_MAP})
    , Form(NJson::TJsonValue{NJson::JSON_MAP})
    , Logger(logger)
    , ShouldLogNlg(request.HasExpFlag(EXP_HW_LOG_NLG))
{
    Context["is_smart_speaker"] = request.ClientInfo().IsSmartSpeaker();
    Context["is_mini_speaker_lg"] = request.ClientInfo().IsMiniSpeakerLG();
    Context["is_mini_speaker_dexp"] = request.ClientInfo().IsMiniSpeakerDexp();
    Context["is_navigator"] = request.ClientInfo().IsNavigator();
    Context["is_auto"] = request.ClientInfo().IsYaAuto();
    Context["is_elari_watch"] = request.ClientInfo().IsElariWatch();
    Context["children_content_restriction"] = request.ContentRestrictionLevel() == EContentSettings::children;
    Context["is_ios"] = request.ClientInfo().IsIOS();
    Context["is_android"] = request.ClientInfo().IsAndroid();
    Context["is_searchapp"] = request.ClientInfo().IsSearchApp();
    Context["is_tv_device"] = request.ClientInfo().IsTvDevice();
    Context["is_legatus"] = request.ClientInfo().IsLegatus();

    ReqInfo["request_id"] = request.RequestId();
}

TNlgData::TNlgData(TRTLogger& logger)
    : Context(NJson::TJsonValue{NJson::JSON_MAP})
    , ReqInfo(NJson::TJsonValue{NJson::JSON_MAP})
    , Form(NJson::TJsonValue{NJson::JSON_MAP})
    , Logger(logger)
{
}

void TNlgData::AddAttention(const TStringBuf attention) {
    Context["attentions"][attention] = true;
}

} // NAlice::NHollywood
