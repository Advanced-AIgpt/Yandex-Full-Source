#pragma once

#include "embedder.h"

#include <alice/hollywood/library/resources/resources.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/protos/data/video/video.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/input.h>

namespace NAlice::NHollywood {

using TSlots = ::google::protobuf::RepeatedPtrField<TSemanticFrame::TSlot>;

using TStringFeature = TMaybe<TString>;

struct TReleaseYearFeature {
    TMaybe<ui32> ExactYear;
    TMaybe<std::pair<ui32, ui32>> YearsRange;

    bool Defined() const {
        return ExactYear || YearsRange;
    }
};

struct TExpectedFeatures {
    TStringFeature Genre;
    TStringFeature Country;
    TReleaseYearFeature ReleaseYear;
    TStringFeature About;
};

TExpectedFeatures ConvertSlotsToFeatures(const TSlots& slots, const TClientInfoProto& clientInfo,
                                         const TMaybe<TString>& filterValue = Nothing());

class TVideoDatabase final : public IResourceContainer {
public:
    struct TItem {
        i32 KinopoiskId = -1;
        TVector<TString> Genres;
        TVector<TString> Countries;
        TVector<TString> Actors;
        TVector<TString> Directors;
        TVector<TString> Keywords;
        TString ContentType;
        i32 MinAge = -1;
        double Rating = 0.;
        ui32 ReleaseYear = 0;
        double LogWatchesCount = 0;
        NJson::TJsonValue VideoItem;
        TVector<float> Embedding;
    };

    void LoadFromPath(const TFsPath& dirPath) override;
    void LoadFromPaths(const TString& videoBasePath, const TString& embedderModelPath,
                       const TString& embedderConfigPath);

    TVector<TVideoItem> Recommend(const TExpectedFeatures& expectedFeatures,
                                  size_t gallerySize, size_t skipCount) const;

    ui32 RecommendableItemCount(const TExpectedFeatures& expectedFeatures) const;

private:
    TVector<TItem> Items;
    TMovieInfoEmbedder Embedder;

private:
    TVector<TVideoItem> RecommendWithTopRating(const TExpectedFeatures& expectedFeatures,
                                               size_t gallerySize, size_t skipCount) const;

    TVector<TVideoItem> RecommendWithTopSimilarity(const TExpectedFeatures& expectedFeatures,
                                                   size_t gallerySize, size_t skipCount) const;
};

} // namespace NAlice::NHollywood
