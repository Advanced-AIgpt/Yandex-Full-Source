#include "sample_mock.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/iterator/zip.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <util/string/cast.h>

namespace NGranet {

using namespace NJson;

namespace NScheme {
    static const TStringBuf FetcherError = "FetcherError";

    namespace NSample {
        static const TStringBuf Text = "Text";
        static const TStringBuf Tokens = "Tokens";
        static const TStringBuf TokenBegin = "TokenBegin";
        static const TStringBuf TokenEnd = "TokenEnd";
        static const TStringBuf FstText = "FstText";
        static const TStringBuf Entities = "Entities";
    }
    namespace NEmbedding {
        static const TStringBuf Type = "Type";
        static const TStringBuf Version = "Version";
        static const TStringBuf Data = "Data";
    }
    namespace NEmbeddings {
        static const TStringBuf Embeddings = "Embeddings";
    }
};

// ~~~~ TSampleMock ~~~~

// static
TSampleMock TSampleMock::FromJson(const TJsonValue& json) {
    TSampleMock mock;
    mock.Text = json[NScheme::NSample::Text].GetStringSafe("");
    mock.Tokens = json[NScheme::NSample::Tokens].GetStringSafe("");
    mock.FstText = json[NScheme::NSample::FstText].GetStringSafe("");
    mock.Entities = LoadEntitiesFromJson(json[NScheme::NSample::Entities]);
    for (const auto& [b, e] : Zip(json[NScheme::NSample::TokenBegin].GetArraySafe(), json[NScheme::NSample::TokenEnd].GetArraySafe())) {
        mock.TokensIntervals.push_back({b.GetUIntegerSafe(), e.GetUIntegerSafe()});
    }
    return mock;
}

TJsonValue TSampleMock::ToJson() const {
    TJsonValue json;
    json[NScheme::NSample::Text] = Text;
    json[NScheme::NSample::Tokens] = Tokens;
    json[NScheme::NSample::FstText] = FstText;
    json[NScheme::NSample::Entities] = SaveEntitiesToJsonCompact(Entities);
    TJsonValue::TArray& beginnings = json[NScheme::NSample::TokenBegin].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& endings = json[NScheme::NSample::TokenEnd].SetType(JSON_ARRAY).GetArraySafe();
    for (const NNlu::TInterval& interval : TokensIntervals) {
        beginnings.push_back(interval.Begin);
        endings.push_back(interval.End);
    }
    return json;
}

// static
TVector<TEntity> FilterEntitiesForMock(const TVector<TEntity>& src) {
    TVector<TEntity> dst(Reserve(src.size()));
    for (const TEntity& entity : src) {
        if (entity.Source != NEntitySources::GRANET) {
            dst.push_back(entity);
        }
    }
    return dst;
}

// static
TSampleMock TSampleMock::FromSample(const TSample::TConstRef& sample, TStringBuf fstText) {
    Y_ENSURE(sample);
    TSampleMock mock;
    mock.Text = sample->GetText();
    mock.Tokens = sample->GetJoinedTokens();
    mock.TokensIntervals = sample->GetTokensIntervals();
    mock.FstText = TString(fstText);
    mock.Entities = FilterEntitiesForMock(sample->GetEntities());
    return mock;
}

// static
TSampleMock TSampleMock::FromBegemotResponse(TStringBuf text, const NJson::TJsonValue& json) {
    TSampleMock mock;
    mock.Text = TString(text);

    if (!json.IsDefined()) {
        return mock; // Empty text after normalization
    }

    const TJsonValue aliceRequestJson = json["rules"]["AliceRequest"];
    Y_ENSURE(aliceRequestJson.IsDefined(), "No AliceRequest results in response");
    const TJsonValue& textJson = aliceRequestJson["OriginalText"];
    mock.Tokens = textJson["NormalizedTokens"].GetStringSafe();
    for (const auto& [b, e] : Zip(textJson["TokenBegin"].GetArraySafe(), textJson["TokenEnd"].GetArraySafe())) {
        mock.TokensIntervals.push_back({b.GetUIntegerSafe(), e.GetUIntegerSafe()});
    }
    mock.FstText = aliceRequestJson["FstText"].GetStringSafe();

    const TJsonValue granetJson = json["rules"]["Granet"];
    Y_ENSURE(granetJson.IsDefined(), "No Granet results in response");
    mock.Entities = FilterEntitiesForMock(LoadEntitiesFromJson(granetJson["AllEntities"]));

    return mock;
}

bool IsSampleMockJsonGood(const TJsonValue& json) {
    return json[NScheme::FetcherError].GetStringSafe("").empty()
        && !json[NScheme::NSample::Text].GetStringSafe("").empty()
        && !json[NScheme::NSample::Tokens].GetStringSafe("").empty()
        && !json[NScheme::NSample::FstText].GetStringSafe("").empty()
        && json[NScheme::NSample::TokenBegin].IsArray()
        && json[NScheme::NSample::TokenEnd].IsArray()
        && json.Has(NScheme::NSample::Entities);
}

bool IsSampleMockStrGood(TStringBuf str) {
    TJsonValue json;
    return ReadJsonTree(str, &json) && IsSampleMockJsonGood(json);
}

// ~~~~ TEmbeddingsMock ~~~~

// static
TEmbeddingMock TEmbeddingMock::FromJson(const TJsonValue& json) {
    TEmbeddingMock mock;
    mock.Type = json[NScheme::NEmbedding::Type].GetStringSafe("");
    mock.Version = json[NScheme::NEmbedding::Version].GetStringSafe("");
    mock.Data = json[NScheme::NEmbedding::Data].GetStringSafe("");
    return mock;
}

TJsonValue TEmbeddingMock::ToJson() const {
    TJsonValue json;
    json[NScheme::NEmbedding::Type] = Type;
    json[NScheme::NEmbedding::Version] = Version;
    json[NScheme::NEmbedding::Data] = Data;
    return json;
}

// static
TEmbeddingsMock TEmbeddingsMock::FromJson(const TJsonValue& json) {
    TEmbeddingsMock mock;
    for (const auto& [key, embeddingJson] : json[NScheme::NEmbeddings::Embeddings].GetMap()) {
        mock.Embeddings[key] = TEmbeddingMock::FromJson(embeddingJson);
    }
    return mock;
}

TJsonValue TEmbeddingsMock::ToJson() const {
    TJsonValue json;
    TJsonValue& embeddingsJson = json[NScheme::NEmbeddings::Embeddings].SetType(JSON_MAP);
    for (const auto& [key, embedding] : Embeddings) {
        embeddingsJson[key] = embedding.ToJson();
    }
    return json;
}

// static
TEmbeddingsMock TEmbeddingsMock::FromBegemotResponse(const TJsonValue& json) {
    TEmbeddingsMock mock;
    if (!json.IsDefined()) {
        return mock; // Empty text after normalization
    }

    for (const TJsonValue& embeddingJson : json["rules"]["AliceEmbeddingsExport"]["Embeddings"].GetArray()) {
        const TString type = embeddingJson[NScheme::NEmbedding::Type].GetStringSafe("");
        if (!type.empty()) {
            mock.Embeddings[type] = TEmbeddingMock::FromJson(embeddingJson);
        }
    }

    return mock;
}

bool IsEmbeddingsMockJsonGood(const TJsonValue& json) {
    return json[NScheme::FetcherError].GetStringSafe("").empty()
        && !json[NScheme::NEmbeddings::Embeddings].GetArray().empty();
}

bool IsEmbeddingsMockStrGood(TStringBuf str) {
    TJsonValue json;
    return ReadJsonTree(str, &json) && IsEmbeddingsMockJsonGood(json);
}

} // namespace NGranet
