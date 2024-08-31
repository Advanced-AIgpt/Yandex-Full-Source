#pragma once

#include <alice/hollywood/library/resources/resources.h>
#include <alice/hollywood/library/scenarios/suggesters/common/recommender_utils.h>

#include <alice/library/json/json.h>
#include <alice/library/util/rng.h>

#include <util/folder/path.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

class TGameRecommender final : public IResourceContainer {
public:
    struct TItem {
        TVector<TResponseNlg> Responses;
        TString Name;
        TString ItemId;

        const TResponseNlg& GetResponse(IRng& rng) const;
    };

    struct TRestrictions {
        THashSet<TString> ItemIds;
    };

    void LoadFromPath(const TFsPath& dirPath) override;
    void LoadFromJson(const NJson::TJsonValue& data);

    const TItem* Recommend(const TRestrictions& restrictions, IRng& rng) const;

private:
    TVector<TItem> Items;
};

}  // namespace NAlice::NHollywood
