#include "search_filter.h"
#include "directives.h"

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

TStringBuf GetCurrentLevel(const TContext& ctx) {
    switch (ctx.GetContentRestrictionLevel()) {
        case EContentRestrictionLevel::Children:
            return NSearchFilter::LEVEL_FAMILY;
        case EContentRestrictionLevel::Without:
            return NSearchFilter::LEVEL_NO_FILTER;
        default:
            return NSearchFilter::LEVEL_MODERATE;
    }
}

void SetLevel(TContext& ctx, const TStringBuf& level) {
    auto currentLevel = GetCurrentLevel(ctx);

    if (currentLevel == level) {
        NSc::TValue errData;
        errData["code"] = "already_set";
        ctx.AddErrorBlock(TError(TError::EType::SEARCHFILTERERROR), errData);
    } else {
        NSc::TValue commandData;
        commandData["new_level"] = level;
        ctx.AddCommand<TSearchFilterSetDirective>(TStringBuf("set_search_filter"), commandData);
    }
}

void AddSuggests(TContext& ctx) {
    ctx.AddSuggest(NSearchFilter::LEVEL_FAMILY);
    ctx.AddSuggest(NSearchFilter::LEVEL_MODERATE);
    ctx.AddSuggest(NSearchFilter::LEVEL_NO_FILTER);
}

TResultValue TSearchFilterFormHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SEARCH_COMMANDS);

    auto clientInfo = ctx.MetaClientInfo();

    if (!ctx.ClientFeatures().SupportsSearchFilterSet() && ctx.FormName() != NSearchFilter::GET_LEVEL) {
        if (clientInfo.IsYaBrowser()) {
            ctx.AddSuggest(TStringBuf("notsupported_yabrowser_url_suggest"));
            ctx.CreateSlot(TStringBuf("result"), TStringBuf("string"), true, TStringBuf("unsupported"));
        } else {
            ctx.AddErrorBlock(
                TError(TError::EType::NOTSUPPORTED, TStringBuf("unsupported_operation")),
                NSc::Null()
            );
        }
        return TResultValue();
    }

    if (ctx.FormName() == NSearchFilter::GET_LEVEL) {
        auto result = GetCurrentLevel(ctx);
        ctx.CreateSlot(TStringBuf("result"), TStringBuf("string"), true, result);
        AddSuggests(ctx);
    } else if (ctx.FormName() == NSearchFilter::HOW_SET_LEVEL) {
        AddSuggests(ctx);
    } else if (ctx.FormName() == NSearchFilter::SET_LEVEL_FAMILY) {
        SetLevel(ctx, NSearchFilter::LEVEL_FAMILY);
    } else if (ctx.FormName() == NSearchFilter::SET_LEVEL_NO_FILTER) {
        SetLevel(ctx, NSearchFilter::LEVEL_NO_FILTER);
    } else if (ctx.FormName() == NSearchFilter::SET_LEVEL_MODERATE) {
        SetLevel(ctx, NSearchFilter::LEVEL_MODERATE);
    }
    return TResultValue();
}

void TSearchFilterFormHandler::Register(THandlersMap* handlers) {
    auto cbSearchFilterForm = []() {
        return MakeHolder<TSearchFilterFormHandler>();
    };
    handlers->emplace(NSearchFilter::SET_LEVEL_FAMILY, cbSearchFilterForm);
    handlers->emplace(NSearchFilter::SET_LEVEL_NO_FILTER, cbSearchFilterForm);
    handlers->emplace(NSearchFilter::SET_LEVEL_MODERATE, cbSearchFilterForm);
    handlers->emplace(NSearchFilter::GET_LEVEL, cbSearchFilterForm);
    handlers->emplace(NSearchFilter::HOW_SET_LEVEL, cbSearchFilterForm);
}

}
