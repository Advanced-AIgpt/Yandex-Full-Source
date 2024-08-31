#pragma once

#include <alice/hollywood/library/modifiers/analytics_info/builder.h>
#include <alice/hollywood/library/modifiers/context/context.h>
#include <alice/hollywood/library/modifiers/external_sources/request_collector.h>
#include <alice/hollywood/library/modifiers/external_sources/response_retriever.h>
#include <alice/hollywood/library/modifiers/response_body_builder/builder.h>

#include <alice/hollywood/library/resources/resources.h>

#include <alice/megamind/protos/modifiers/modifier_request.pb.h>
#include <alice/megamind/protos/modifiers/modifier_response.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>

namespace NAlice::NHollywood::NModifiers {

inline constexpr TStringBuf EXP_ENABLE_MODIFIER_PREFIX = "mm_enable_modifier=";

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

    explicit TNonApply(EType type);
    explicit TNonApply(const TString& reason);
    EType Type() const;
    TString Reason() const;

private:
    EType Type_;
    TMaybe<TString> Reason_;
};

using TApplyResult = TMaybe<TNonApply>;

struct TModifierPrepareContext {
    IModifierContext& ModifierContext;
    const TModifierBody& RequestBody;
    IExternalSourceRequestCollector& ExternalSourcesRequestCollector;
};

struct TModifierApplyContext {
    IModifierContext& ModifierContext;
    TResponseBodyBuilder& ResponseBody;
    TModifierAnalyticsInfoBuilder& AnalyticsInfo;
    TExternalSourcesResponseRetriever ExternalSourcesResponseRetriever;
};

class TBaseModifier {

public:
    virtual ~TBaseModifier() = default;

    explicit TBaseModifier(const TString& modifierType)
        : ModifierType_(modifierType)
    {
    }

    virtual void LoadResourcesFromPath(const TFsPath& modifierResourcesBasePath) {
        Y_UNUSED(modifierResourcesBasePath);
    }

    virtual void Prepare(TModifierPrepareContext prepareCtx) const {
        Y_UNUSED(prepareCtx);
    }

    virtual TApplyResult TryApply(TModifierApplyContext applyCtx) const = 0;

    const TString& GetModifierType() const {
        return ModifierType_;
    }

    void SetEnabled(bool enabled);
    bool IsEnabled(const IModifierContext& ctx) const;
private:

    const TString ModifierType_;
    bool Enabled_ = false;
};

using TBaseModifierPtr = std::unique_ptr<TBaseModifier>;

} // namespace NAlice::NHollywood::NModifiers
