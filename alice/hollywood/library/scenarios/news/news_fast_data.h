#pragma once

#include <alice/hollywood/library/scenarios/news/proto/news_fast_data.pb.h>

#include <alice/hollywood/library/fast_data/fast_data.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/json/json.h>
#include <alice/library/util/rng.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood {

struct TSmi {
    TString Name;
    TString NameAccusative;
    TString GranetId;
    TString MementoId;
    TString ApiId;
    TString Uri;
    bool IsMementable;
    TString Logo;

    TSmi(TSmiProto proto) {
        Name = proto.GetName();
        NameAccusative = proto.GetNameAccusative();
        GranetId = proto.GetGranetId();
        MementoId = proto.GetMementoId();
        ApiId = proto.GetApiId();
        Uri = proto.GetUri();
        IsMementable = proto.GetMementable();
        Logo = proto.GetLogo();
    }

    TSmi(TString name, TString nameAccusative, TString granetId,
        TString mementoId, TString apiId, TString uri, bool isMementable, TString logo) :
        Name(name),
        NameAccusative(nameAccusative),
        GranetId(granetId),
        MementoId(mementoId),
        ApiId(apiId),
        Uri(uri),
        IsMementable(isMementable),
        Logo(logo)
    {
    }
};

struct TNewsPostroll {
    NScenarios::TFrameAction FrameAction;
    TString Postroll;
    bool IsEnable;
    TString EnableFlag;
    TString DisableFlag;
    int ProbaScore;
    bool HasFrameAction;

    TNewsPostroll(TNewsPostrollProto proto) {
        FrameAction = proto.GetFrameAction();
        HasFrameAction = !FrameAction.GetFrame().GetName().is_null();
        Postroll = proto.GetPostroll();
        IsEnable = proto.GetIsEnable();
        EnableFlag = proto.GetEnableFlag();
        DisableFlag = proto.GetDisableFlag();
        ProbaScore = Max(proto.GetProbaScore(), 1);
    }

    bool IsEnabled(const TScenarioRunRequestWrapper& request) const;
};

class TNewsFastData : public IFastData {
public:
    TNewsFastData(const TVector<TSmi>& smiCollection);
    TNewsFastData(const TNewsFastDataProto& proto);
    const TSmi* GetSmiByGranetId(TString granetId) const;
    const TSmi* GetSmiByMementoId(TString mementoId) const;
    bool IsMementableSmi(TString granetId) const;
    bool HasSmi(TString granetId) const;
    bool HasMementoId(TString mementoId) const;

    int GetPostrollsCount(const TScenarioRunRequestWrapper& request) const;
    const TNewsPostroll* GetRandomPostroll(const TScenarioRunRequestWrapper& request, NAlice::IRng& rng) const;

private:
    THashMap<TString, TSmi> SmiByGranetId;
    THashMap<TString, TSmi> SmiByMementoId;

    TVector<TNewsPostroll> RadioNewsPostrolls;
};

} // namespace NAlice::NHollywood
