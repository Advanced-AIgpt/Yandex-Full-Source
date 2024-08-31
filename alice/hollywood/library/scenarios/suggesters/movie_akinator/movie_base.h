#pragma once

#include <alice/hollywood/library/resources/resources.h>

#include <library/cpp/json/json_value.h>

#include <util/digest/numeric.h>
#include <util/folder/path.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/str_stl.h>
#include <util/stream/output.h>

namespace NAlice::NHollywood {

struct TMovieInfo {
    TString Name;
    ui32 KinopoiskId = 0;
    TString OntoId;
    ui32 Popularity = 0;
    ui32 MinAge = 0;
    TString ContentType;
    TVector<TString> Genres;
    TString OpenUrl;
    TString CoverUrl;
    TString Description;
};

struct TNode {
    size_t Index = NPOS;
    size_t LeftChildIndex = NPOS;
    size_t RightChildIndex = NPOS;
    size_t ParentIndex = NPOS;
    size_t ClusterSize = 0;
    bool HasPosterCloud = false;
    TMaybe<TString> LeftPosterCloudUrl;
    TMaybe<TString> RightPosterCloudUrl;
    TVector<size_t> MovieInfoIndices;
    ui64 Hash = 0;
};

struct TTree {
    TVector<TNode> Nodes;
    TVector<size_t> InitialSuggestNodeIndices;
    THashMap<TString, size_t> OntoIdToCluster;
};

struct TFilterCondition {
    TMaybe<TString> ExpectedContentType;
    TMaybe<TString> ExpectedGenre;

    bool operator==(const TFilterCondition& other) const {
        return ExpectedContentType == other.ExpectedContentType && ExpectedGenre == other.ExpectedGenre;
    }

    bool operator!=(const TFilterCondition& other) const {
        return !(*this == other);
    }
};

struct TContentFilterSuggest {
    TFilterCondition Condition;
    TString ContentName;
};

} // namespace NAlice::NHollywood

template <>
class THash<NAlice::NHollywood::TFilterCondition> {
public:
    inline size_t operator()(const NAlice::NHollywood::TFilterCondition& otherCondition) const {
        return CombineHashes(FieldHash(otherCondition.ExpectedContentType),
                             FieldHash(otherCondition.ExpectedGenre));
    }

private:
    THash<TMaybe<TString>> FieldHash;
};

template <>
inline void Out<NAlice::NHollywood::TFilterCondition>(
    IOutputStream& out, const NAlice::NHollywood::TFilterCondition& value)
{
    if (value.ExpectedContentType) {
        out << "Content type: " << value.ExpectedContentType << " ";
    }
    if (value.ExpectedGenre) {
        out << "Genre: " << value.ExpectedGenre << " ";
    }
}

namespace NAlice::NHollywood {

struct TClusteredMovies final : public IResourceContainer {
    TVector<TMovieInfo> MovieInfos;
    THashMap<TString, size_t> OntoIdToMovieInfoIndex;

    THashMap<TFilterCondition, TTree> Trees;
    TVector<TContentFilterSuggest> ContentFilterSuggests;

    THashSet<size_t> DiscussableMovieIndices;

    TVector<size_t> GetFilteredMovieInfoIndices(const TFilterCondition& condition, size_t maxSize) const;
    void LoadFromPath(const TFsPath& dirPath) override;
};

} // namespace NAlice::NHollywood
