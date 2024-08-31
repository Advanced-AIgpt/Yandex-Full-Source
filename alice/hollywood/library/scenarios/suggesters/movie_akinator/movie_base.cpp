#include "movie_base.h"

#include <alice/library/json/json.h>

#include <library/cpp/resource/resource.h>

#include <util/digest/city.h>
#include <util/digest/numeric.h>
#include <util/generic/algorithm.h>
#include <util/generic/is_in.h>
#include <util/generic/strbuf.h>
#include <util/stream/file.h>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf DATA_PATH = "clustered_movies.json";

constexpr size_t MIN_SUGGESTED_CLUSTER = 5;
constexpr size_t MAX_SUGGESTED_CLUSTER = 150;

void ParseStringArray(const NJson::TJsonValue& jsonArray, TVector<TString>& values) {
    for (const auto& jsonElement : jsonArray.GetArray()) {
        if (!jsonElement.IsString()) {
            continue;
        }
        values.push_back(jsonElement.GetString());
    }
}

TMovieInfo ParseMovieInfo(const NJson::TJsonValue& jsonData) {
    TMovieInfo movieInfo;

    movieInfo.Name = jsonData["name"].GetString();
    movieInfo.ContentType = jsonData["content_type"].GetString();
    movieInfo.OntoId = jsonData["onto_id"].GetString();
    movieInfo.OpenUrl = jsonData["kinopoisk_url"].GetString();
    movieInfo.CoverUrl = jsonData["cover_url"].GetString();
    movieInfo.Description = jsonData["description"].GetString();

    movieInfo.KinopoiskId = jsonData["kinopoisk_id"].GetInteger();
    movieInfo.Popularity = jsonData["popularity"].GetInteger();
    movieInfo.MinAge = jsonData["min_age"].GetInteger();

    ParseStringArray(jsonData["genres"], movieInfo.Genres);

    return movieInfo;
}

TVector<TMovieInfo> LoadMovieInfos(const NJson::TJsonValue& movieInfosJson) {
    TVector<TMovieInfo> movieInfos;

    for (const NJson::TJsonValue& movieInfoJson : movieInfosJson.GetArray()) {
        movieInfos.push_back(ParseMovieInfo(movieInfoJson));
    }

    return movieInfos;
}

void ProcessChild(size_t childIndex, TNode& node, TVector<TNode>& nodes, TVector<size_t>& nodeMovieInfoIndices) {
    if (childIndex == NPOS) {
        return;
    }

    auto& child = nodes.at(childIndex);
    child.ParentIndex = node.Index;

    nodeMovieInfoIndices.insert(nodeMovieInfoIndices.end(), child.MovieInfoIndices.begin(),
                                child.MovieInfoIndices.end());
}

ui64 CalculateNodeHash(const TVector<TMovieInfo>& movieInfos, const TVector<size_t>& nodeMovieInfoIndices) {
    ui64 hash = 0;
    for (size_t movieInfoIndex : nodeMovieInfoIndices) {
        hash = CombineHashes(hash, CityHash64(movieInfos.at(movieInfoIndex).Name));
    }

    return hash;
}

void AppendNode(const NJson::TJsonValue& jsonData, const TVector<TMovieInfo>& movieInfos, TVector<TNode>& nodes) {
    TNode node;

    node.Index = jsonData["index"].GetInteger();
    node.ClusterSize = jsonData["subtree_size"].GetInteger();

    if (node.ClusterSize == 1) {
        const auto movieIndex = jsonData["movie_info_index"].GetInteger();
        node.MovieInfoIndices.push_back(movieIndex);
    } else {
        TVector<size_t> nodeMovieInfoIndices;

        node.LeftChildIndex = jsonData["left_child_index"].GetInteger();
        ProcessChild(node.LeftChildIndex, node, nodes, nodeMovieInfoIndices);

        node.RightChildIndex = jsonData["right_child_index"].GetInteger();
        ProcessChild(node.RightChildIndex, node, nodes, nodeMovieInfoIndices);

        if (jsonData["left_image_url"].IsDefined()) {
            node.LeftPosterCloudUrl = jsonData["left_image_url"].GetString();
        }
        if (jsonData["right_image_url"].IsDefined()) {
            node.RightPosterCloudUrl = jsonData["right_image_url"].GetString();
        }

        Y_ENSURE(node.LeftPosterCloudUrl.Defined() == node.RightPosterCloudUrl.Defined());
        node.HasPosterCloud = node.LeftPosterCloudUrl.Defined();

        SortBy(nodeMovieInfoIndices, [&movieInfos](const auto& movieInfoIndex){
            return -movieInfos.at(movieInfoIndex).Popularity;
        });
        node.MovieInfoIndices = nodeMovieInfoIndices;

        node.Hash = CalculateNodeHash(movieInfos, nodeMovieInfoIndices);
    }

    Y_ENSURE(node.Index == nodes.size());
    nodes.push_back(std::move(node));
}

void CollectInitialSuggestNodeIndices(const TNode& node, const TVector<TNode>& nodes,
                                      TVector<size_t>& initialSuggestClusters,
                                      size_t minClusterSize = MIN_SUGGESTED_CLUSTER,
                                      size_t maxClusterSize = MAX_SUGGESTED_CLUSTER)
{
    if (node.HasPosterCloud && minClusterSize <= node.ClusterSize && node.ClusterSize <= maxClusterSize) {
        initialSuggestClusters.push_back(node.Index);
        return;
    }

    if (node.LeftChildIndex != NPOS) {
        CollectInitialSuggestNodeIndices(nodes.at(node.LeftChildIndex), nodes, initialSuggestClusters,
                                         minClusterSize, maxClusterSize);
    }
    if (node.RightChildIndex != NPOS) {
        CollectInitialSuggestNodeIndices(nodes.at(node.RightChildIndex), nodes, initialSuggestClusters,
                                         minClusterSize, maxClusterSize);
    }
}

TTree LoadTree(const NJson::TJsonValue& treeJson, const TVector<TMovieInfo>& movieInfos) {
    TTree tree;

    for (const auto& nodeJson : treeJson.GetArray()) {
        AppendNode(nodeJson, movieInfos, tree.Nodes);
    }
    Y_ENSURE(!tree.Nodes.empty());

    CollectInitialSuggestNodeIndices(tree.Nodes.back(), tree.Nodes, tree.InitialSuggestNodeIndices);

    for (size_t nodeIndex : tree.InitialSuggestNodeIndices) {
        for (size_t movieInfoIndex : tree.Nodes[nodeIndex].MovieInfoIndices) {
            tree.OntoIdToCluster[movieInfos[movieInfoIndex].OntoId] = nodeIndex;
        }
    }

    return tree;
}

void LoadTrees(const NJson::TJsonValue& treesJson, const TVector<TMovieInfo>& movieInfos,
               THashMap<TFilterCondition, TTree>& trees, TVector<TContentFilterSuggest>& contentFilterSuggests)
{
    for (const auto& treeInfoJson : treesJson.GetArray()) {
        const TTree tree = LoadTree(treeInfoJson["tree_nodes"], movieInfos);

        TFilterCondition condition;
        for (const auto& [key, value] : treeInfoJson["filter"].GetMap()) {
            if (key == "content_type") {
                condition.ExpectedContentType = value.GetString();
            } else if (key == "genre") {
                condition.ExpectedGenre = value.GetString();
            }
        }
        trees[condition] = tree;

        if (tree.InitialSuggestNodeIndices.size() <= 1) {
            continue;
        }

        if (treeInfoJson["content_name"].IsString()) {
            TContentFilterSuggest filterSuggest;
            filterSuggest.Condition = condition;
            filterSuggest.ContentName = treeInfoJson["content_name"].GetString();

            contentFilterSuggests.push_back(std::move(filterSuggest));
        }
    }
}

THashSet<ui32> LoadDiscussableMovieKinopoiskIds(const TFsPath& dirPath) {
    TFileInput fileStream(dirPath / "known_movies.json");

    THashSet<ui32> kinopoiskIds;
    for (TString line; fileStream.ReadLine(line);) {
        NJson::TJsonValue value;
        const bool readCorrectly = NJson::ReadJsonFastTree(line, &value);
        Y_ENSURE(readCorrectly);

        kinopoiskIds.insert(value["id"].GetUIntegerSafe());
    }

    return kinopoiskIds;
}

THashSet<size_t> LoadDiscussableMovieIndices(const TFsPath& dirPath, const TVector<TMovieInfo>& movieInfos) {
    const THashSet<ui32> discussableKinopoiskIds = LoadDiscussableMovieKinopoiskIds(dirPath);

    THashSet<size_t> discussableMovieIndices;
    for (size_t i = 0; i < movieInfos.size(); ++i) {
        if (discussableKinopoiskIds.contains(movieInfos[i].KinopoiskId)) {
            discussableMovieIndices.insert(i);
        }
    }

    return discussableMovieIndices;
}

} // namespace

void TClusteredMovies::LoadFromPath(const TFsPath& dirPath) {
    TFileInput inputStream(dirPath / DATA_PATH);
    const NJson::TJsonValue jsonData = JsonFromString(inputStream.ReadAll());

    MovieInfos = LoadMovieInfos(jsonData["movie_infos"]);
    for (size_t movieInfoIndex = 0; movieInfoIndex < MovieInfos.size(); ++movieInfoIndex) {
        OntoIdToMovieInfoIndex[MovieInfos[movieInfoIndex].OntoId] = movieInfoIndex;
    }

    LoadTrees(jsonData["trees"], MovieInfos, Trees, ContentFilterSuggests);
    Y_ENSURE(!ContentFilterSuggests.empty());

    DiscussableMovieIndices = LoadDiscussableMovieIndices(dirPath, MovieInfos);
}

TVector<size_t> TClusteredMovies::GetFilteredMovieInfoIndices(const TFilterCondition& condition, size_t maxSize) const {
    TVector<size_t> movieInfoIndices;

    for (size_t movieInfoIndex = 0; movieInfoIndex < MovieInfos.size(); ++movieInfoIndex) {
        const auto& movieInfo = MovieInfos[movieInfoIndex];

        if (condition.ExpectedContentType && condition.ExpectedContentType != movieInfo.ContentType) {
            continue;
        }
        if (condition.ExpectedGenre && !IsIn(movieInfo.Genres, condition.ExpectedGenre)) {
            continue;
        }

        movieInfoIndices.push_back(movieInfoIndex);
    }

    SortBy(movieInfoIndices, [&](size_t movieInfoIndex){ return -MovieInfos[movieInfoIndex].Popularity; });
    if (movieInfoIndices.size() > maxSize) {
        movieInfoIndices.resize(maxSize);
    }

    return movieInfoIndices;
}

} // namespace NAlice::NHollywood
