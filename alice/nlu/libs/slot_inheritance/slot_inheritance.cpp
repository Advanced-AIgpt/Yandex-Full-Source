#include "slot_inheritance.h"

namespace NAlice {
    namespace {
        constexpr TStringBuf STRING_TYPE = "string";
        constexpr NNlu::TInterval ZERO_INTERVAL{0, 0};

        bool IsEntitySuitable(
            const TSlotDescription& slotDescription,
            const TInheritanceEntity& candidateSlot,
            bool checkSlotName
        ) {
            return (!candidateSlot.UsedInInheritance && candidateSlot.SlotValue.Type != STRING_TYPE) &&
                   (candidateSlot.SlotName == slotDescription.SlotName || !checkSlotName);
        }

        TSlotTypeToSlotsValues ReformatFormForInheritance(const TForm& form) {
            TSlotTypeToSlotsValues formForInheritance;
            for (const auto& [slotName, slotValue] : form.Slots) {
                for (auto slotEntity : slotValue.Data) {
                    slotEntity.Interval = ZERO_INTERVAL;
                    TInheritanceEntity inheritanceEntity{slotName, slotEntity};
                    formForInheritance[slotEntity.Type].push_back(inheritanceEntity);
                }
            }
            return formForInheritance;
        }
    } // anonymous namespace

    TSlotReuser::TSlotReuser(const TForm& previousForm, TForm currentForm, TOrderedFormDescription formDescription)
        : FormHypothesis(std::move(currentForm))
        , EntityTypeToPreviousFormSlots(ReformatFormForInheritance(previousForm))
        , FormDescription(std::move(formDescription))
    {
    }

    void TSlotReuser::FinishInheritance(bool checkSlotName) {
        if (checkSlotName && !InheritanceMode.InheritOnlyBySlotName) {
            InheritByType();
        } else {
            Result.push_back(FormHypothesis);
        }
    }

    void TSlotReuser::FillSlotRecursive(size_t currentSlotPosition, bool checkSlotName) {
        if (currentSlotPosition >= FormDescription.size()) {
            FinishInheritance(checkSlotName);
            return;
        }
        if (FormHypothesis.Slots.contains(FormDescription[currentSlotPosition].SlotName)) {
            FillSlotRecursive(currentSlotPosition + 1, checkSlotName);
            return;
        }
        bool slotHasBeenFilled = false;
        for (const auto& type : FormDescription[currentSlotPosition].AcceptedTypes) {
            const auto& slotCandidates = EntityTypeToPreviousFormSlots.find(type);
            if (slotCandidates == EntityTypeToPreviousFormSlots.end()) {
                continue;
            }
            for (auto& candidateValue : slotCandidates->second) {
                if (!IsEntitySuitable(FormDescription[currentSlotPosition], candidateValue, checkSlotName)) {
                    continue;
                }
                slotHasBeenFilled = true;
                candidateValue.UsedInInheritance = true;
                const NGranet::TResultSlot currentSlot{ZERO_INTERVAL,
                                                       FormDescription[currentSlotPosition].SlotName,
                                                       {candidateValue.SlotValue}};
                FormHypothesis.Slots.emplace(FormDescription[currentSlotPosition].SlotName, currentSlot);
                FillSlotRecursive(currentSlotPosition + 1, checkSlotName);
                candidateValue.UsedInInheritance = false;
                FormHypothesis.Slots.erase(FormDescription[currentSlotPosition].SlotName);
            }
        }
        if (!slotHasBeenFilled || InheritanceMode.AllowSkipSlots) {
            FillSlotRecursive(currentSlotPosition + 1, checkSlotName);
        }
    }

    void TSlotReuser::InheritByType() {
        FillSlotRecursive(/* currentSlotPosition= */ 0, /* checkSlotName= */ false);
    }

    void TSlotReuser::InheritByNameAndType() {
        FillSlotRecursive(/* currentSlotPosition= */ 0, /* checkSlotName= */ true);
    }

    void TSlotReuser::FilterResults() {
        EraseIf(Result, [&](const TForm& inheritedForm) -> bool {
            // If any slot is inherited, number of slots in form increases
            return FormHypothesis.Slots.size() == inheritedForm.Slots.size();
        });
        if (InheritanceMode.ReturnFirstPriority && !Result.empty()) {
            Result.resize(1);
        }
    }

    TVector<TForm> TSlotReuser::Apply(TInheritanceMode inheritanceMode) {
        Result.clear();
        InheritanceMode = std::move(inheritanceMode);
        InheritByNameAndType();
        FilterResults();

        return Result;
    }
} // namespace NAlice
