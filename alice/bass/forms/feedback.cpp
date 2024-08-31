#include "feedback.h"

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NBASS {

namespace {

using TSuggestList = TVector<TStringBuf>;
using TFeedbackTypes = THashMap<TStringBuf, TSuggestList>;

const TFeedbackTypes& FeedbackTypes() {
    static const THashMap<TStringBuf, TSuggestList> feedbackTypes = {
        { "personal_assistant.feedback.feedback_positive",
            {
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.feedback_negative",
            {
                "feedback_negative__bad_answer",
                "feedback_negative__asr_error",
                "feedback_negative__tts_error",
                "feedback_negative__offensive_answer",
                "feedback_negative__other",
                "feedback_negative__all_good",
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.gc_feedback_positive",
            { }
        },
        { "personal_assistant.feedback.gc_feedback_negative",
            { }
        },
        { "personal_assistant.feedback.gc_feedback_negative__reason",
            {
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.gc_feedback_neutral__reason",
            {
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.gc_feedback_neutral",
            { }
        },
        { "personal_assistant.feedback.feedback_negative__bad_answer",
            {
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.feedback_negative__asr_error",
            {
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.feedback_negative__tts_error",
            {
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.feedback_negative__offensive_answer",
            {
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.feedback_negative__other",
            {
                "search_internet_fallback"
            }
        },
        { "personal_assistant.feedback.feedback_negative__all_good",
            {
                "search_internet_fallback"
            }
        }
    };

    return feedbackTypes;
}

} // anon namespace

TResultValue TFeedbackFormHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::FEEDBACK);
    const TSuggestList* suggests = FeedbackTypes().FindPtr(r.Ctx().FormName());
    if (Y_UNLIKELY(!suggests)) {
        return TResultValue();
    }

    for (TStringBuf suggestType: *suggests) {
        if (suggestType == "search_internet_fallback") {
            r.Ctx().AddSearchSuggest();
            r.Ctx().AddOnboardingSuggest();
        } else {
            NSc::TValue formUpdate;
            if (suggestType.StartsWith("feedback_negative__")) {
                formUpdate["name"] = TString("personal_assistant.feedback.") + suggestType;
            }
            r.Ctx().AddSuggest(suggestType, NSc::Null(), formUpdate);
        }
    }

    return TResultValue();
}

void TFeedbackFormHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TFeedbackFormHandler>();
    };

    for (const auto& kv : FeedbackTypes()) {
        handlers->emplace(kv.first, handler);
    }
}

} // namespace NBASS
