#pragma once

#include <alice/hollywood/library/nlg/nlg_data.h>
#include <library/cpp/langs/langs.h>

#include <alice/megamind/protos/scenarios/analytics_info.pb.h>

namespace NAlice::NHollywood {

class TNlgRenderHistoryRecordStorage : TMoveOnly {
public:
    explicit TNlgRenderHistoryRecordStorage(const bool collectFullNlgRenderContext);

    void TrackRenderPhrase(const TStringBuf templateName, const TStringBuf phraseName, const TNlgData& nlgData, const ELanguage language);
    void TrackRenderCard(const TStringBuf templateName, const TStringBuf cardName, const TNlgData& nlgData, const ELanguage language);

    const TVector<NScenarios::TAnalyticsInfo::TNlgRenderHistoryRecord>& GetTrackedRecords() const {
        return Records_;
    }

private:
    NScenarios::TAnalyticsInfo::TNlgRenderHistoryRecord& TrackNlgRenderHistoryRecord(
        const TStringBuf templateName, const TNlgData& nlgData, const ELanguage language);
private:
    bool CollectFullNlgRenderContext_ = false;
    TVector<NScenarios::TAnalyticsInfo::TNlgRenderHistoryRecord> Records_;
};

}
