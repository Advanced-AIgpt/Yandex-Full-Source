#pragma once

#include "collected_entities.h"

#include <alice/nlu/granet/lib/sample/entity.h>

#include <alice/library/music/defs.h>
#include <alice/library/request/slot.h>
#include <alice/library/video_common/defs.h>

#include <alice/megamind/protos/common/frame.pb.h>

#include <search/begemot/rules/alice/tagger/proto/alice_tagger.pb.h>
#include <search/begemot/rules/granet/proto/granet.pb.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

    // Position in the list corresponds to priority in the descent order
    inline const THashMap<TStringBuf, TVector<TString>> SLOT_VALUES_PRIORITY_LIST = {
        {NVideoCommon::SLOT_SELECTION_ACTION_TYPE, {ToString(NVideoCommon::ESelectionAction::Description)}},
    };

    class TTaggerPrediction {
    public:
        TTaggerPrediction(
            TStringBuf taggerName,
            const ::google::protobuf::RepeatedPtrField<NBg::NProto::TAliceTaggerPrediction>& protoPredictions,
            const TCollectedEntities& entities,
            const TFrameDescription* frameDescription
        );

        const TString& GetName() const;
        const TSlotMap& GetSlotMap() const;

        TMaybe<double> GetProbability() const {
            return Probability;
        }

    private:
        TString Name;
        TMaybe<double> Probability;
        TSlotMap SlotMap;
    };

    TVector<TString> ExtractSourceTokens(const NBg::NProto::TAliceTaggerPrediction& prediction);

    void FillFormAndFrame(
        const NBg::NProto::TAliceTaggerPredictions& predictions,
        const TVector<NGranet::TEntity>& granetEntities,
        const TCollectedEntities& collectedEntities,
        const TString& formName,
        const TFrameDescription* frameDescription,
        const THashSet<TString>& intentsToForceRegularProcessing,
        NBg::NProto::TGranetForm* form,
        TSemanticFrame* frame
    );

    void FillFormsAndFrames(
        const ::google::protobuf::Map<TString, NBg::NProto::TAliceTaggerPredictions>& taggerPredictions,
        const TVector<NGranet::TEntity>& entities,
        const NBg::NProto::TCustomEntitiesResult& customEntities,
        const NBg::NProto::TAliceEntitiesCollectorResult& aliceEntitiesCollectorResult,
        const THashMap<TString, TFrameDescription>& frameDescriptionMap,
        const THashSet<TString>& intentsToForceRegularProcessing,
        NBg::NProto::TAliceTaggerResult* result
    );
}
