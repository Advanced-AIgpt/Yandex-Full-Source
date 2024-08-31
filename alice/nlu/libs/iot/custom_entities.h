#pragma once

#include <alice/library/iot/structs.h>

#include <alice/nlu/granet/lib/sample/entity.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <alice/nlu/libs/entity_searcher/entity_searcher_types.h>

#include <alice/megamind/protos/common/iot.pb.h>

#include <kernel/lemmer/core/language.h>


namespace NAlice::NIot {

constexpr double EXACT_BONUS = 1;

constexpr double EXACT_QUALITY = 0;
constexpr double LEMMA_QUALITY = -1;
constexpr double EXACT_CLOSE_VARIATION_QUALITY = -2;
constexpr double CLOSE_VARIATION_QUALITY = -3;
constexpr double EXACT_SYNONYM_QUALITY = -4;
constexpr double SYNONYM_QUALITY = -5;

enum EIoTEntityQuality {
    ITEQ_LEMMA = 0,
    ITEQ_CLOSE_VARIATION = 1,
    ITEQ_SYNONYM = 2,
};

inline EIoTEntityQuality GetEntityQualityType(double quality) {
    return static_cast<EIoTEntityQuality>(static_cast<int>(-quality) / 2);
}

inline bool IsExactEntityQuality(double quality) {
    constexpr double exactQualities[] = {EXACT_QUALITY, EXACT_CLOSE_VARIATION_QUALITY, EXACT_SYNONYM_QUALITY};
    return IsIn(exactQualities, quality);
}


TVector<NNlu::TEntityString> ParseIoTConfig(const NAlice::TIoTUserInfo& ioTUserInfo, ELanguage lang);

TVector<NGranet::TEntity> FindIoTEntities(const TVector<NNlu::TEntityString>& ioTEntitiesStrings,
                                          TVector<TString> tokens, const ELanguage lang);

TVector<TRawEntity> MakeRawEntities(const TVector<NGranet::TEntity>& entities, const TVector<TString>& tokens);

}