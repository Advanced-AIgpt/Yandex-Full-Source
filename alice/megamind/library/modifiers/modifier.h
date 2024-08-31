#pragma once

#include "context.h"

#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/util/status.h>

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

// Can be either common with an enum type or mod specific with a reason message
class TNonApply {
public:
    enum class EType {
        DisabledByFlag /* "disabled by flag" */,
        DisabledInApp /* "disabled in the app" */,
        DisabledDuringPureGC /* "forbidden during pure gc" */,
        DisabledWhileListening /* "forbidden while listening" */,
        NotApplicable /* "not applicable to response" */,
        ModSpecific /* "modifier specific fail" */,
    };

    explicit TNonApply(EType type)
        : Type_(type)
    {
    }

    explicit TNonApply(const TString& reason)
        : Type_(EType::ModSpecific)
        , Reason_(reason)
    {
    }

    EType Type() const {
        return Type_;
    }

    TString Reason() const {
        if (Reason_) {
            return *Reason_;
        } else if (Type_ != EType::ModSpecific) {
            return ToString(Type_);
        }
        return TString{};
    }

private:
    EType Type_;
    TMaybe<TString> Reason_;
};

using TMaybeNonApply = TMaybe<TNonApply>;
using TApplyResult = TErrorOr<TMaybeNonApply>;

inline TMaybeNonApply ApplySuccess() {
    return Nothing();
}

inline TMaybeNonApply NonApply(TNonApply::EType type) {
    return TNonApply(type);
}

inline TMaybeNonApply NonApply(const TString& reason) {
    return TNonApply(reason);
}

class TResponseModifier {
public:
    virtual ~TResponseModifier() = default;

    virtual TApplyResult TryApply(TResponseModifierContext& ctx, TScenarioResponse& response) = 0;

    const TString& GetName() const {
        return Name;
    }

protected:
    explicit TResponseModifier(TStringBuf name)
        : Name(TString{name})
    {
    }

private:
    const TString Name; // for logs
};

using TModifierPtr = THolder<NMegamind::TResponseModifier>;

} // NAlice::NMegamind
