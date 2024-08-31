#pragma once

#include <alice/megamind/protos/common/frame.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

struct TSlot {
    class TValue {
    public:
        TValue() = default;

        explicit TValue(const TString& value)
            : Value(value) {
        }

        explicit TValue(TString&& value)
            : Value(std::move(value)) {
        }

        bool operator==(const TValue& rhs) const {
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

    TString Name;
    TString Type;
    TValue Value;
    TVector<TString> AcceptedTypes = {};
    bool IsRequested = false;
    bool IsFilled = false;
};

TSemanticFrame::TSlot CreateProtoSlot(const TString& name, const TString& type, const TString& value);

} // namespace NAlice::NHollywood
