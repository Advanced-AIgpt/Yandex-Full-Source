#pragma once

#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>

namespace NAlice::NHollywood::NModifiers {

constexpr TStringBuf AH_ITEM_POLYGLOT_REQUEST_NAME = "polyglot_request";
constexpr TStringBuf AH_ITEM_POLYGLOT_HTTP_REQUEST_NAME = "polyglot_http_request";
constexpr TStringBuf AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME = "polyglot_http_response";

class TPolyglotModifier : public TBaseModifier {
public:
    TPolyglotModifier();

    void Prepare(TModifierPrepareContext prepareCtx) const override;
    TApplyResult TryApply(TModifierApplyContext applyCtx) const override;
};

} // namespace NAlice::NHollywood::NModifiers
