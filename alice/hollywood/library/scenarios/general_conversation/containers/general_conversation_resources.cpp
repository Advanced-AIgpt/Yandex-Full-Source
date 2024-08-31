#include "general_conversation_resources.h"

#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>

#include <alice/hollywood/library/gif_card/gif_card.h>

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/iterator/zip.h>
#include <library/cpp/yaml/as/tstring.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

NAlice::TBoltalkaDssmEmbedder LoadEmbedder(const TFsPath& dirPath, TStringBuf modelName) {
    auto modelDir = dirPath / "embedders" / modelName;

    TFileInput modelStream(modelDir / "embedder");
    auto model = TBlob::FromString(modelStream.ReadAll());
    TFileInput configStream(modelDir / "config.json");
    auto config = NAlice::TBoltalkaDssmEmbedder::LoadConfig(&configStream);

    return {std::move(model), std::move(config)};
}

TMemoryRanker LoadMemoryRanker(const TFsPath& dirPath, TStringBuf modelName) {
    auto modelDir = dirPath / "rankers" / modelName;

    TFileInput modelStream(modelDir / "dssm.pb");
    TFileInput modelConfigStream(modelDir / "dssm_config.json");
    TFileInput rankerConfigStream(modelDir / "ranker_config.json");

    return {&modelStream, &modelConfigStream, &rankerConfigStream};
}

THashMap<TString, NAlice::TBoltalkaDssmEmbedder> LoadEmbedders(const TFsPath& dirPath) {
    THashMap<TString, NAlice::TBoltalkaDssmEmbedder> embedders;

    embedders.emplace(ToString(NLU_SEARCH_MODEL_NAME), LoadEmbedder(dirPath, NLU_SEARCH_MODEL_NAME));
    embedders.emplace(ToString(NLU_SEARCH_CONTEXT_MODEL_NAME), LoadEmbedder(dirPath, NLU_SEARCH_CONTEXT_MODEL_NAME));

    return embedders;
}

THashMap<TString, TMemoryRanker> LoadMemoryRankers(const TFsPath& dirPath) {
    THashMap<TString, TMemoryRanker> memoryRankers;

    memoryRankers.emplace(ToString(LSTM256_MODEL_NAME), LoadMemoryRanker(dirPath, LSTM256_MODEL_NAME));

    return memoryRankers;
}

TVector<TString> LoadYamlStringVector(const YAML::Node& node) {
    TVector<TString> result;
    if (node && node.IsSequence()) {
        for (const auto& element: node) {
            result.push_back(element.as<TString>());
        }
    }
    return result;
}

THashMap<TString, TMicrointent::TMusicInfo> LoadNlgActionsFromSequence(const YAML::Node& intentNlg) {
    THashMap<TString, TMicrointent::TMusicInfo> nlgActions;
    if (!intentNlg.IsSequence()) {
        return nlgActions;
    }
    for (const auto& reply : intentNlg) {
        if (reply.IsMap()) {
            nlgActions.emplace(reply["answer"].as<TString>(),
                               TMicrointent::TMusicInfo{reply["query"].as<TString>()});
        }
    }
    return nlgActions;

}

THashMap<TString, TMicrointent::TMusicInfo> LoadNlgActions(const YAML::Node& intentNlg) {
    if (intentNlg.IsSequence()) {
        return LoadNlgActionsFromSequence(intentNlg);
    }
    THashMap<TString, TMicrointent::TMusicInfo> nlgActions;
    if (!intentNlg.IsMap()) {
        return nlgActions;
    }
    for (const auto& it : intentNlg) {
        auto actions = LoadNlgActionsFromSequence(it.second);
        nlgActions.insert(actions.begin(), actions.end());
    }
    return nlgActions;
}

TMicrointent LoadMicrointent(const YAML::Node& intentInfoNode, bool shouldLoadNlu, float defaultThreshold) {
    float threshold = defaultThreshold;
    if (intentInfoNode["threshold"]) {
        threshold = intentInfoNode["threshold"].as<float>();
    }
    float modalThreshold = threshold;
    if (intentInfoNode["pure_threshold"]) {
        modalThreshold = intentInfoNode["pure_threshold"].as<float>();
    }
    TVector<TString> suggests = LoadYamlStringVector(intentInfoNode["suggests"]);
    TVector<TVector<TString>> ledImages;
    if (intentInfoNode["show_led_gif"] && intentInfoNode["show_led_gif"].IsSequence()) {
        for (const auto& element: intentInfoNode["show_led_gif"]) {
            ledImages.push_back(LoadYamlStringVector(element));
        }
    }
    bool isGcFallback = intentInfoNode["gc_fallback"] && intentInfoNode["gc_fallback"].as<bool>();
    bool ShouldNotListen = intentInfoNode["should_not_listen"] && intentInfoNode["should_not_listen"].as<bool>();
    TString emotion;
    if (intentInfoNode["emotion"]) {
        emotion = intentInfoNode["emotion"].as<TString>();
    }
    bool isBithday = intentInfoNode["birthday"] && intentInfoNode["birthday"].as<bool>();
    TVector<TString> nlu;
    if (shouldLoadNlu) {
        nlu = LoadYamlStringVector(intentInfoNode["nlu"]);
    }
    bool isAllowed = intentInfoNode["is_allowed"] && intentInfoNode["is_allowed"].as<bool>();
    return {threshold, modalThreshold, suggests, ledImages,
            isGcFallback, ShouldNotListen, emotion, isBithday,
            nlu, TVector<TString>{}, LoadNlgActions(intentInfoNode["nlg"]), isAllowed};
}

THashMap<TString, TMicrointent> LoadMicrointents(const TFsPath& dirPath) {
    THashMap<TString, TMicrointent> config;

    TFileInput fileStream(dirPath / "ru_microintents.yaml");
    YAML::Node rootNode = YAML::Load(fileStream.ReadAll());

    const float defaultThreshold = rootNode["default_threshold"].as<float>();
    for (const auto& intentInfoNode : rootNode["intents"]) {
        const TString intent = intentInfoNode.first.as<TString>();
        config.emplace(intent, LoadMicrointent(intentInfoNode.second, false, defaultThreshold));
    }
    for (const auto& intentInfoNode : rootNode["ellipsis_intents"]) {
        const TString intent = intentInfoNode.first.as<TString>();
        config.emplace(intent, LoadMicrointent(intentInfoNode.second, true, defaultThreshold));
        TVector<TString> previousIntents = LoadYamlStringVector(intentInfoNode.second["allowed_prev_intents"]);
        for (const auto& previousIntentName : previousIntents) {
            if (auto* previousIntent = config.FindPtr(previousIntentName)) {
                previousIntent->EllipsisIntents.push_back(intent);
            }
        }
    }

    return config;
}

TVector<TGif> LoadUnclassifiedGifs(const TFsPath& dirPath) {
    TVector<TGif> gifs;
    TFileInput fileStream(dirPath / "unclassified_gifs.json");

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(&fileStream, &jsonConfig);
    Y_ENSURE(readCorrectly);
    for (const auto& node : jsonConfig.GetArraySafe()) {
        gifs.push_back(GifFromJson(node));
    }
    return gifs;
}

THashMap<TString, TVector<TGif>> LoadEmotionalGifs(const TFsPath& dirPath) {
    THashMap<TString, TVector<TGif>> gifDescriptions;
    TFileInput fileStream(dirPath / "emoji" / "gifs.json");

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(&fileStream, &jsonConfig);
    Y_ENSURE(readCorrectly);
    for (const auto& node : jsonConfig.GetArraySafe()) {
        gifDescriptions[node["emoji"].GetStringSafe()].push_back(GifFromJson(node));
    }
    for (const auto& [key, value] : gifDescriptions) {
        Y_ENSURE(!value.empty());
    }
    return gifDescriptions;
}

THashMap<TString, TVector<TString>> LoadEmotionalLedImages(const TFsPath& dirPath) {
    THashMap<TString, TVector<TString>> ledImagesDescriptions;
    TFileInput fileStream(dirPath / "emoji" / "led_images.json");

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(&fileStream, &jsonConfig);
    Y_ENSURE(readCorrectly);
    for (const auto& node : jsonConfig.GetArraySafe()) {
        ledImagesDescriptions[node["emoji"].GetStringSafe()].push_back(node["url"].GetStringSafe());
    }
    for (const auto& [key, value] : ledImagesDescriptions) {
        Y_ENSURE(!value.empty());
    }
    return ledImagesDescriptions;
}


THolder<::NNlg::TEmojiClassifier> LoadEmojiClassifier(const TFsPath& dirPath) {
    TFileInput model(dirPath / "emoji" / "model.pb");
    TFileInput emojiConfig(dirPath / "emoji" / "emoji_config.json");
    TFileInput modelConfig(dirPath / "emoji" / "model_config.json");
    return MakeHolder<::NNlg::TEmojiClassifier>(&model, &modelConfig, &emojiConfig);
}

THashMap<TString, ui32> LoadOntoToKpIdMapping(const TFsPath& dirPath) {
    TFileInput fileStream(dirPath / "onto_to_kp_id_mapping.json");

    NJson::TJsonValue jsonMapping;
    const bool readCorrectly = NJson::ReadJsonTree(&fileStream, &jsonMapping);
    Y_ENSURE(readCorrectly);

    THashMap<TString, ui32> mapping;
    for (const auto& [key, value] : jsonMapping.GetMapSafe()) {
        mapping[key] = value.GetUIntegerSafe();
    }

    return mapping;
}

THashMap<TString, TEntity> LoadKnownMovies(const TFsPath& dirPath) {
    TFileInput fileStream(dirPath / "known_movies.json");

    THashMap<TString, TEntity> mapping;
    for (TString line; fileStream.ReadLine(line);) {
        NJson::TJsonValue value;
        const bool readCorrectly = NJson::ReadJsonFastTree(line, &value);
        Y_ENSURE(readCorrectly);

        TEntity entity;
        TMovie& movie = *entity.MutableMovie();
        movie.SetId(value["id"].GetUIntegerSafe());
        movie.SetTitle(value["title"].GetStringSafe());
        movie.SetType(value["type"].GetStringSafe());
        movie.SetNegativeAnswerFraction(value["questions"]["negatives_fraction"].GetDoubleSafe());

        mapping.emplace(GetMovieEntityKey(value["id"].GetUIntegerSafe()), std::move(entity));
    }

    return mapping;
}

TVector<TMovie> LoadMoviesToDiscuss(const TFsPath& dirPath, const THashMap<TString, TEntity>& knownEntities) {
    TFileInput fileStream(dirPath / "movies_to_discuss.json");

    TVector<TMovie> movies;
    for (TString line; fileStream.ReadLine(line);) {
        NJson::TJsonValue value;
        const bool readCorrectly = NJson::ReadJsonFastTree(line, &value);
        Y_ENSURE(readCorrectly);

        const auto entityKey = GetMovieEntityKey(value["id"].GetUIntegerSafe());
        const auto* entity = knownEntities.FindPtr(entityKey);
        Y_ENSURE(entity, entityKey);
        movies.push_back(entity->GetMovie());
    }

    return movies;
}

THashMap<TString, TVector<TMovie>> GetMoviesByType(const TVector<TMovie>& movies) {
    THashMap<TString, TVector<TMovie>> result;

    for (const auto& movie : movies) {
        auto moviesByType = result.FindPtr(movie.GetType());
        if (!moviesByType) {
            moviesByType = &result.emplace(movie.GetType(), TVector<TMovie>{}).first->second;
        }
        moviesByType->push_back(movie);
    }

    return result;
}

void CheckTypes(const THashMap<TString, TVector<TMovie>>& moviesByType) {
    for (const auto& [movieType, _] : moviesByType) {
        Y_ENSURE(KNOWN_MOVIE_CONTENT_TYPES.contains(movieType), movieType);
    }

    for (const auto& movieType : KNOWN_MOVIE_CONTENT_TYPES) {
        Y_ENSURE(moviesByType.FindPtr(movieType), movieType);
    }
}

using TProtoStringArray = ::google::protobuf::RepeatedPtrField<TProtoStringType>;

void LoadStringArray(const NJson::TJsonValue& jsonArray, TVector<TString>& values) {
    for (const auto& jsonElement : jsonArray.GetArraySafe()) {
        if (!jsonElement.IsString()) {
            continue;
        }
        values.push_back(jsonElement.GetStringSafe());
    }
}

THashMap<TString, TSentimentConditionedQuestions> LoadEntityQuestions(const TFsPath& dirPath) {
    TFileInput fileStream(dirPath / "known_movies.json");

    THashMap<TString, TSentimentConditionedQuestions> entityToQuestions;
    for (TString line; fileStream.ReadLine(line);) {
        NJson::TJsonValue value;
        const bool readCorrectly = NJson::ReadJsonFastTree(line, &value);
        Y_ENSURE(readCorrectly);

        const ui32 id = value["id"].GetUIntegerSafe();

        TSentimentConditionedQuestions questions;
        LoadStringArray(value["questions"]["positives"], questions[TEntityDiscussion::POSITIVE]);
        LoadStringArray(value["questions"]["negatives"], questions[TEntityDiscussion::NEGATIVE]);

        entityToQuestions.emplace(GetMovieEntityKey(id), std::move(questions));
    }

    return entityToQuestions;
}

void CheckLoadedModels(const THashMap<TString, NAlice::TBoltalkaDssmEmbedder>& embedders, const THashMap<TString, TMemoryRanker>& memoryRankers) {
    for (const auto& [rankerName, rankerModel] : memoryRankers) {
        Y_ENSURE(embedders.contains(rankerModel.GetEmbedderName()));
    }
}

THashMap<TString, TVector<TString>> LoadCrossPromo(const TFsPath& dirPath, TStringBuf jsonName) {
    TVector<THashMap<TString, TVector<TString>>> data;
    TFileInput fileStream(dirPath / jsonName );

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(&fileStream, &jsonConfig);
    Y_ENSURE(readCorrectly);
    THashMap<TString, TVector<TString>> mapping;
    for (const auto& [key, value] : jsonConfig.GetMapSafe()) {
        TVector<TString> promo_facts;
        for (const auto& fact : value.GetArraySafe()) {
            promo_facts.push_back(fact.GetStringSafe());
        }
        if (!promo_facts.empty()) {
            mapping[key] = promo_facts;
        }
    }
    return mapping;
}

THolder<NBg::TMicrointentsIndex> LoadMicrointentsClassifier(const TFsPath& dirPath) {
    auto modelDir = dirPath / "embedders" / NLU_SEARCH_MODEL_NAME;

    TFileInput modelStream(modelDir / "embedder");
    auto model = TBlob::FromString(modelStream.ReadAll());
    TFileInput configStream(dirPath / "ru_dummy_microintents.yaml");
    return MakeHolder<NBg::TMicrointentsIndex>(model, configStream.ReadAll(), ELanguage::LANG_RUS);;
}

} // namespace

void TGeneralConversationResources::LoadFromPath(const TFsPath& dirPath) {
    Embedders_ = LoadEmbedders(dirPath);
    MemoryRankers_ = LoadMemoryRankers(dirPath);
    Microintents_ = LoadMicrointents(dirPath);
    EmotionalGifs_ = LoadEmotionalGifs(dirPath);
    EmotionalLedImages_ = LoadEmotionalLedImages(dirPath);
    EmojiClassifier_ = LoadEmojiClassifier(dirPath);
    OntoToKpIdMapping_ = LoadOntoToKpIdMapping(dirPath);
    UnclassifiedGifs_ = LoadUnclassifiedGifs(dirPath);
    KnownEntities_ = LoadKnownMovies(dirPath);
    EntityToQuestions_ = LoadEntityQuestions(dirPath);
    CrossPromoFacts_ = LoadCrossPromo(dirPath, "crosspromo_general_conversation.json");
    CrossPromoFactsFiltered_ = LoadCrossPromo(dirPath, "crosspromo_general_conversation_filtered.json");
    MoviesToDiscuss_ = LoadMoviesToDiscuss(dirPath, KnownEntities_);
    MoviesToDiscussByType_ = GetMoviesByType(MoviesToDiscuss_);
    MicrointentsClassifier_ = LoadMicrointentsClassifier(dirPath);
    ClusteredMovies_.LoadFromPath(dirPath);

    CheckTypes(MoviesToDiscussByType_);

    CheckLoadedModels(Embedders_, MemoryRankers_);
}

const NAlice::TBoltalkaDssmEmbedder* TGeneralConversationResources::GetEmbedder(TStringBuf name) const {
    return Embedders_.FindPtr(name);
}

const TMemoryRanker* TGeneralConversationResources::GetRanker(TStringBuf name) const {
    return MemoryRankers_.FindPtr(name);
}

const THashMap<TString, TMicrointent>& TGeneralConversationResources::GetMicrointents() const {
    return Microintents_;
}

const THashMap<TString, TVector<TGif>>& TGeneralConversationResources::GetEmotionalGifs() const {
    return EmotionalGifs_;
}

const THashMap<TString, TVector<TString>>& TGeneralConversationResources::GetEmotionalLedImages() const {
    return EmotionalLedImages_;
}

const ::NNlg::TEmojiClassifier* TGeneralConversationResources::GetEmojiClassifier() const {
    return EmojiClassifier_.Get();
}

const THashMap<TString, ui32>& TGeneralConversationResources::GetOntoToKpIdMapping() const {
    return OntoToKpIdMapping_;
}

const TVector<TGif>& TGeneralConversationResources::GetUnclassifiedGifs() const {
    return UnclassifiedGifs_;
}

const TEntity* TGeneralConversationResources::GetEntity(const TString& id) const {
    return KnownEntities_.FindPtr(id);
}

const TVector<TString>* TGeneralConversationResources::GetEntityQuestions(
    const TString& id, TEntityDiscussion::EDiscussionSentiment sentiment) const
{
    if (const auto* sentimentConditionedQuestions = EntityToQuestions_.FindPtr(id)) {
        if (const auto* questions = sentimentConditionedQuestions->FindPtr(sentiment)) {
            return questions;
        }
    }
    return nullptr;
}

const THashMap<TString, TVector<TString>>& TGeneralConversationResources::GetCrossPromoFacts(bool useFilteredDict) const {
    if (useFilteredDict) {
        return CrossPromoFactsFiltered_;
    }

    return CrossPromoFacts_;
}

const TVector<TMovie>& TGeneralConversationResources::GetMoviesToDiscuss() const {
    return MoviesToDiscuss_;
}

const TVector<TMovie>& TGeneralConversationResources::GetMoviesToDiscussByType(const TString& movieType) const {
    return *MoviesToDiscussByType_.FindPtr(movieType);
}

const TClusteredMovies& TGeneralConversationResources::GetClusteredMovies() const {
    return ClusteredMovies_;
}

const NBg::TMicrointentsIndex* TGeneralConversationResources::GetMicrointentsClassifier() const {
    return MicrointentsClassifier_.Get();
}

} // namespace NAlice::NHollywood::NGeneralConversation
