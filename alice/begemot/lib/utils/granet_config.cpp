#include "granet_config.h"

namespace NBg {

NProto::TGranetConfig MakeGranetConfig(const NGranet::TMultiGrammar::TConstRef& grammar) {
    NProto::TGranetConfig destConfig;
    for (const auto& [key, task] : grammar->GetTasks()) {
        if (key.Type == NGranet::PTT_FORM) {
            const NGranet::TParserTask& srcForm = *task.Task;
            NProto::TGranetConfig::TForm* destForm = destConfig.AddForms();
            destForm->SetName(key.Name);
            destForm->SetEnableGranetParser(srcForm.EnableGranetParser);
            destForm->SetEnableAliceTagger(srcForm.EnableAliceTagger);
            for (const NGranet::TSlotDescription& srcSlot : srcForm.Slots) {
                NProto::TGranetConfig::TSlot* destSlot = destForm->AddSlots();
                destSlot->SetName(srcSlot.Name);
                for (const TString& type : srcSlot.DataTypes) {
                    destSlot->AddAcceptedTypes(type);
                }
                destSlot->SetMatchingType(ToString<NGranet::ESlotMatchingType>(srcSlot.MatchingType));
                destSlot->SetConcatenateStrings(srcSlot.ConcatenateStrings);
                destSlot->SetKeepVariants(srcSlot.KeepVariants);
            }
        } else if (key.Type == NGranet::PTT_ENTITY) {
            const NGranet::TParserTask& srcEntity = *task.Task;
            NProto::TGranetConfig::TEntity* destEntity = destConfig.AddEntities();
            destEntity->SetName(key.Name);
            destEntity->SetKeepOverlapped(srcEntity.KeepOverlapped);
        }
    }

    return destConfig;
}

} // namespace NBg
