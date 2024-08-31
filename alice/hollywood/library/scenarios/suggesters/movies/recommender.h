#pragma once

#include <alice/hollywood/library/resources/resources.h>

#include <alice/library/json/json.h>
#include <alice/library/restriction_level/restriction_level.h>
#include <alice/library/util/rng.h>
#include <alice/protos/data/video/video.pb.h>

#include <util/folder/path.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

class TMovieRecommender final : public IResourceContainer {
public:
    struct TItem {
        TVector<TString> Texts;
        TMaybe<TString> PersuadingText;
        TString Name;
        TString ItemId;
        bool IsPornoGenre = false;
        ui32 MinAge = 0;
        TString ContentType;
        THashSet<TString> BassGenres;
        TVideoItem VideoItem;

        const TString& GetText(bool isPersuasionStep, IRng& rng) const;
    };

    struct TRestrictions {
        EContentRestrictionLevel Age = EContentRestrictionLevel::Without;
        THashSet<TString> ItemIds;
        TMaybe<TString> ContentType;
        TMaybe<TString> Genre;
    };

    void LoadFromPath(const TFsPath& dirPath) override;
    void LoadFromJson(const NJson::TJsonValue& data);

    const TItem* Recommend(const TRestrictions& restrictions, IRng& rng) const;

    const TItem* GetItemById(const TString& itemId) const;

private:
    TVector<TItem> Items;
};

}  // namespace NAlice::NHollywood
