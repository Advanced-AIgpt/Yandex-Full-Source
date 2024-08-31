#pragma once

#include "aligned_entities.h"

#include <alice/nlu/proto/entities/fst.pb.h>

#include <search/begemot/rules/alice/request/proto/common.pb.h>

namespace NBg::NAliceEntityCollector {

TVector<TString> CollectFstEntities(const ::google::protobuf::RepeatedPtrField<NAlice::NNlu::TFstEntity>& srcEntities,
                        std::function<void(NGranet::TEntity)> onConverted);
void CollectFstEntities(const ::google::protobuf::RepeatedPtrField<NAlice::NNlu::TFstEntity>& srcEntities,
                        TAlignedEntities* alignedEntities, bool IsPASkills);

void CollectAliceEntities(const ::google::protobuf::RepeatedPtrField<NProto::TAliceEntity>& srcEntities,
                          const TString& text, TAlignedEntities* alignedEntities);

} // namespace NBg::NAliceEntityCollector
