#pragma once

#include "token_range.h"

#include <alice/megamind/protos/common/frame.pb.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/string/cast.h>

namespace NAlice {

struct TSlot {
    class TSlotValue {
    public:
        TSlotValue() = default;
        explicit TSlotValue(const TString& value)
            : Value(value) {
        }

        bool operator==(const TSlotValue& rhs) const {
            return Value == rhs.Value;
        }

        const TString& AsString() const {
            return Value;
        }

        template <typename TResult>
        TMaybe<TResult> As() const {
            TResult result;
            if (!TryFromString(Value, result)) {
                return Nothing();
            }
            return result;
        }

    private:
        TString Value;
    };

    TSlot() = default;
    TSlot(const TString& name, const TString& type, const TString& value,
          const TMaybe<TString>& sourceText = Nothing(), const TTokenRange& range = TTokenRange{});

    bool operator==(const TSlot& rhs) const {
        return Name == rhs.Name && Type == rhs.Type && Value == rhs.Value && SourceText == rhs.SourceText;
    }

    TString Name;
    TString Type;
    TSlotValue Value;
    TMaybe<TString> SourceText;
    TTokenRange Range;
};

using TSlotMap = THashMap<TString, TSlot>;

inline bool IsSlotEmpty(const TSlot* slot) {
    return !slot || slot->Value.AsString().empty();
}

TSemanticFrame ToSemanticFrame(const TString& name, const TSlotMap& slotMap);

} // namespace NAlice
