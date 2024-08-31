#pragma once

#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NAlice {

inline constexpr TStringBuf SLOT_STRING_TYPE = "string";
inline constexpr TStringBuf SLOT_NUM_TYPE = "num";
inline constexpr TStringBuf SLOT_FST_NUM_TYPE = "fst.num";
inline constexpr TStringBuf SLOT_DATE_TYPE = "date";
inline constexpr TStringBuf SLOT_VARIANTS_TYPE = "variants";

inline constexpr TStringBuf FST_TYPE_PREFIX = "fst.";
inline constexpr TStringBuf CUSTOM_TYPE_PREFIX = "custom.";

struct TFrameDescription {
    struct TSlot {
        TSlot(const TString& name, const TVector<TString>& types, bool concatenateStrings = false, bool keepVariants = false)
            : Name(name)
            , Types(types)
            , ConcatenateStrings(concatenateStrings)
            , KeepVariants(keepVariants) {
        }

        TSlot(TStringBuf name, const TVector<TStringBuf>& types, bool concatenateStrings = false, bool keepVariants = false)
            : Name(TString{name})
            , Types(Reserve(types.size()))
            , ConcatenateStrings(concatenateStrings)
            , KeepVariants(keepVariants) {
            for (TStringBuf type : types) {
                Types.push_back(TString{type});
            }
        }

        TString Name;
        TVector<TString> Types;
        bool ConcatenateStrings = false;
        bool KeepVariants = false;
    };

    TFrameDescription() = default;
    TFrameDescription(std::initializer_list<TSlot> slots) {
        for (const auto& slot : slots) {
            AddSlot(slot);
        }
    }

    TFrameDescription& AddSlot(const TSlot& slot) {
        Slots.emplace(slot.Name, slot);
        return *this;
    }

    THashMap<TString, TSlot> Slots;
};

using TFrameDescriptionMap = THashMap<TString, TFrameDescription>;

} // namespace NAlice
