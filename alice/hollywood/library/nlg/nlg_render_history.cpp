#include "nlg_render_history.h"
#include <alice/library/json/json.h>

namespace NAlice::NHollywood {

namespace {

// Original req_info can contain sensitive data (tokens), so we select only required fields. Feel free to add some fields if needed.
NJson::TJsonValue SelectReqInfoNlgRequiredFields(const NJson::TJsonValue& reqInfo) {
    static const TStringBuf requiredFields[] = {"request_id", "experiments", "utterance"};
    const auto& reqInfoMap = reqInfo.GetMap();
    NJson::TJsonValue result;
    for (const auto requiredField : requiredFields) {
        if (const auto* value = reqInfoMap.FindPtr(requiredField)) {
            result[requiredField] = *value;
        }
    }
    return result;
}

}

TNlgRenderHistoryRecordStorage::TNlgRenderHistoryRecordStorage(const bool collectFullNlgRenderContext)
    : CollectFullNlgRenderContext_(collectFullNlgRenderContext)
{
}

void TNlgRenderHistoryRecordStorage::TrackRenderPhrase(const TStringBuf templateName, const TStringBuf phraseName,
                                                    const TNlgData& nlgData, const ELanguage language)
{
    TrackNlgRenderHistoryRecord(templateName, nlgData, language)
        .SetPhraseName(phraseName.cbegin(), phraseName.size());
}

void TNlgRenderHistoryRecordStorage::TrackRenderCard(const TStringBuf templateName, const TStringBuf cardName,
                                                    const TNlgData& nlgData, const ELanguage language)
{
    TrackNlgRenderHistoryRecord(templateName, nlgData, language)
        .SetCardName(cardName.cbegin(), cardName.size());
}

NScenarios::TAnalyticsInfo::TNlgRenderHistoryRecord& TNlgRenderHistoryRecordStorage::TrackNlgRenderHistoryRecord(
    const TStringBuf templateName, const TNlgData& nlgData, const ELanguage language)
{
    auto& nlgRenderHistoryRecord = Records_.emplace_back();
    nlgRenderHistoryRecord.SetTemplateName(templateName.cbegin(), templateName.size());
    nlgRenderHistoryRecord.SetLanguage(static_cast<::NAlice::ELang>(language));
    if (CollectFullNlgRenderContext_) {
        *nlgRenderHistoryRecord.MutableContext() = JsonToProto<::google::protobuf::Struct>(nlgData.Context);
        *nlgRenderHistoryRecord.MutableReqInfo() = JsonToProto<::google::protobuf::Struct>(SelectReqInfoNlgRequiredFields(nlgData.ReqInfo));
        *nlgRenderHistoryRecord.MutableForm() = JsonToProto<::google::protobuf::Struct>(nlgData.Form);
    }
    return nlgRenderHistoryRecord;
}

}
