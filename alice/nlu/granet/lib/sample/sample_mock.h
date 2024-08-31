#pragma once

#include "entity.h"
#include "sample.h"
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet {

// ~~~~ TSampleMock ~~~~

struct TSampleMock {
    TString Text;
    TString Tokens;
    TVector<NNlu::TInterval> TokensIntervals;
    TString FstText;
    TVector<TEntity> Entities;

    static TSampleMock FromJson(const NJson::TJsonValue& json);
    NJson::TJsonValue ToJson() const;

    static TSampleMock FromSample(const TSample::TConstRef& sample, TStringBuf fstText);
    static TSampleMock FromBegemotResponse(TStringBuf text, const NJson::TJsonValue& json);
};

bool IsSampleMockJsonGood(const NJson::TJsonValue& json);
bool IsSampleMockStrGood(TStringBuf str);

// ~~~~ TEmbeddingsMock ~~~~

struct TEmbeddingMock {
    TString Type;
    TString Version;
    TString Data;

    static TEmbeddingMock FromJson(const NJson::TJsonValue& json);
    NJson::TJsonValue ToJson() const;
};

struct TEmbeddingsMock {
    TMap<TString, TEmbeddingMock> Embeddings;

    static TEmbeddingsMock FromJson(const NJson::TJsonValue& json);
    NJson::TJsonValue ToJson() const;

    static TEmbeddingsMock FromBegemotResponse(const NJson::TJsonValue& json);
};

bool IsEmbeddingsMockJsonGood(const NJson::TJsonValue& json);
bool IsEmbeddingsMockStrGood(TStringBuf str);

} // namespace NGranet
