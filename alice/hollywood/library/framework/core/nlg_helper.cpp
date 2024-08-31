#include "nlg_helper.h"

namespace NAlice::NHollywoodFw::NPrivate {

NHollywood::TNlgData ConstructNlgData(const TRequest& request,
    const NJson::TJsonValue& jsonContext,
    const NJson::TJsonValue& complexRoot)
{
    NHollywood::TNlgData nlgData(request.Debug().Logger());
    nlgData.Context = jsonContext;

    nlgData.ShouldLogNlg = request.Flags().IsExperimentEnabled(NHollywood::EXP_HW_LOG_NLG);
    nlgData.Context["is_smart_speaker"] = request.Client().GetClientInfo().IsSmartSpeaker();
    nlgData.Context["is_mini_speaker_lg"] = request.Client().GetClientInfo().IsMiniSpeakerLG();
    nlgData.Context["is_mini_speaker_dexp"] = request.Client().GetClientInfo().IsMiniSpeakerDexp();
    nlgData.Context["is_navigator"] = request.Client().GetClientInfo().IsNavigator();
    nlgData.Context["is_auto"] = request.Client().GetClientInfo().IsYaAuto();
    nlgData.Context["is_elari_watch"] = request.Client().GetClientInfo().IsElariWatch();
    nlgData.Context["is_ios"] = request.Client().GetClientInfo().IsIOS();
    nlgData.Context["is_android"] = request.Client().GetClientInfo().IsAndroid();
    nlgData.Context["is_searchapp"] = request.Client().GetClientInfo().IsSearchApp();
    nlgData.Context["is_tv_device"] = request.Client().GetClientInfo().IsTvDevice();
    nlgData.ReqInfo["request_id"] = request.System().RequestId();

    // TODO [DD] children_content_restriction changes source:
    // WAS: https://a.yandex-team.ru/arc_vcs/alice/library/restriction_level/protos/content_settings.proto
    //      + https://a.yandex-team.ru/arc_vcs/alice/megamind/protos/common/device_state.proto?rev=r9052065#L405
    // NEW: https://a.yandex-team.ru/arc_vcs/alice/megamind/protos/scenarios/request.proto?rev=r9040723#L501
    // nlgData.Context["children_content_restriction"] = (request.User().GetContentRestrictionLevel() == EContentRestrictionLevel::Children);
    nlgData.Context["children_content_restriction"] = (request.User().IsSafeMode());

    for (const auto& it : complexRoot.GetMap()) {
        nlgData.Context[it.first] = it.second;
    }
    return nlgData;
}

} // namespace NAlice::NHollywoodFw::NPrivate
