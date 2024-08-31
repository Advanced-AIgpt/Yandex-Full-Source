#pragma once

#include <alice/nlu/granet/lib/granet.h>
#include <alice/library/frame/description.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {
    struct TSlotDescription {
        TString SlotName;
        TVector<TString> AcceptedTypes;
    };

    using TOrderedFormDescription = TVector<TSlotDescription>;

    struct TForm {
        TString FormName;
        THashMap<TString, NGranet::TResultSlot> Slots;
    };

    struct TInheritanceEntity {
        TString SlotName;
        NGranet::TResultSlotValue SlotValue;
        bool UsedInInheritance = false;
    };

    struct TInheritanceMode {
        bool ReturnFirstPriority = false;
        bool InheritOnlyBySlotName = false;
        bool AllowSkipSlots = false;
    };

    using TSlotTypeToSlotsValues = THashMap<TString, TVector<TInheritanceEntity>>;

    class TSlotReuser {
    public:
        TSlotReuser(const TForm& oldForm, TForm currentForm, TOrderedFormDescription formDescription);
        TVector<TForm> Apply(TInheritanceMode inheritanceMode);

    private:
        void RecursiveFillSlots();
        void InheritByType();
        void FillSlotRecursive(size_t currentSlotPosition, bool checkSlotName);
        void InheritByNameAndType();
        void FinishInheritance(bool checkSlotName);
        void AddToResults();
        void FilterResults();

    private:
        TInheritanceMode InheritanceMode;
        TVector<TForm> Result;
        TForm FormHypothesis;
        TSlotTypeToSlotsValues EntityTypeToPreviousFormSlots;
        TOrderedFormDescription FormDescription;
    };
} // namespace NAlice
