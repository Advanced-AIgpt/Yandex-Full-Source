#include "send_bugreport.h"
#include "directives.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/string/builder.h>

namespace NBASS {

namespace {

constexpr TStringBuf BUGREPORT_FLAG = "debug_mode";
constexpr TStringBuf BUGREPORT_IS_NOT_SUPPORTED = "not_supported";
constexpr TStringBuf BUGREPORT_OPEN_LINK = "open_link";

static constexpr TStringBuf BUGREPORT_START = "personal_assistant.internal.bugreport";
static constexpr TStringBuf BUGREPORT_CONTINUE = "personal_assistant.internal.bugreport__continue";
static constexpr TStringBuf BUGREPORT_FINISH = "personal_assistant.internal.bugreport__deactivate";

}

TResultValue TSendBugreportFormHandler::Do(TRequestHandler &r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::BUGREPORT);

    if (ctx.HasExpFlag(BUGREPORT_FLAG)) {
        if (ctx.FormName() == BUGREPORT_START) {
            NSc::TValue payload;
            TStringBuf reqid = ctx.ReqId();
            payload["id"].SetString(reqid);
            payload["listening_is_possible"] = 1;

            auto* slotReportId = ctx.GetOrCreateSlot(TStringBuf("report_id"), TStringBuf("string"));
            slotReportId->Value.SetString(reqid);

            ctx.AddCommand<TBugReportSendDirective>(TStringBuf("send_bug_report"), std::move(payload));
        } else if (ctx.FormName() == BUGREPORT_CONTINUE) {
            TStringBuilder text;

            auto* slotBuffer = ctx.GetOrCreateSlot(TStringBuf("buffer"), TStringBuf("string"));
            if (!IsSlotEmpty(slotBuffer)) {
                text << slotBuffer->Value.GetString();
                text << ". ";
            }

            TUtf16String utterance = UTF8ToWide(ctx.Meta().Utterance());
            utterance.to_title();
            text << utterance;

            slotBuffer->Value.SetString(text);
        } else if (ctx.FormName() == BUGREPORT_FINISH) {
            // Just fix user message in our logs
            auto* slotReportId = ctx.GetSlot(TStringBuf("report_id"));
            auto* slotBuffer = ctx.GetSlot(TStringBuf("buffer"));
            if (!IsSlotEmpty(slotReportId) && !IsSlotEmpty(slotBuffer)) {
                LOG(DEBUG) << "USER BUGREPORT ID: " << slotReportId->Value.GetString() << ", MESSAGE: " << slotBuffer->Value.GetString() << Endl;
            }
        }
    } else if (ctx.ClientFeatures().SupportsOpenLink()) {
        NSc::TValue data;
        data["uri"].SetString(TStringBuf("https://yandex.ru/support/alice/feedback.html"));
        ctx.AddSuggest(TStringBuf("feedback"), data);
        ctx.AddCommand<TOpenFeedbackDirective>(TStringBuf("open_uri"), data);
        ctx.AddAttention(BUGREPORT_OPEN_LINK);
    } else {
        ctx.AddAttention(BUGREPORT_IS_NOT_SUPPORTED);
    }

    return TResultValue();
}

void TSendBugreportFormHandler::Register(THandlersMap *handlers) {
    auto handler = []() {
        return MakeHolder<TSendBugreportFormHandler>();
    };
    handlers->emplace(BUGREPORT_START, handler);
    handlers->emplace(BUGREPORT_CONTINUE, handler);
    handlers->emplace(BUGREPORT_FINISH, handler);
}

}
