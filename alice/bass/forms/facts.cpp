#include "facts.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/app_host/context.h>
#include <alice/bass/libs/app_host/runner.h>
#include <alice/bass/libs/facts/app_host.h>

namespace NBASS {

namespace {
constexpr TStringBuf FACTS_SCENARIO = "personal_assistant.scenarios.facts_apphost";
constexpr TStringBuf FACTS_GTA =
    "results[0]/binary/SUGGESTFACTS2/data/Grouping[0]/Group[0]/Document[0]/ArchiveInfo/GtaRelatedAttribute"sv
;
}

TResultValue TFactsHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();

    TFactsAppHostSource factsSource(ctx.Meta().Utterance(),
                                    ctx.Meta().Tld(),
                                    ctx.MetaLocale().Lang);

    TAppHostInitContext appHostInitContext;
    appHostInitContext.AddSourceInit(factsSource);

    TAppHostRunner appHostRunner(ctx.GetSources().AliceGraph());
    TAppHostResultContext appHostResultContext = appHostRunner.Fetch(appHostInitContext);

    TMaybe<NSc::TValue> factsAnswer = appHostResultContext.GetItemRef("SUGGESTFACTS2_APPHOST");
    if (!factsAnswer.Defined()) {
        return TResultValue();
    }

    const NSc::TValue& factsGta = factsAnswer.Get()->TrySelect(FACTS_GTA);

    NSc::TValue factSerpData;
    for (const auto& item : factsGta.GetArray()) {
        if (item["Key"].GetString() == "_SerpData") {
            factSerpData = NSc::TValue::FromJson(item["Value"].GetString());
            break;
        }
    }

    if (!factSerpData.IsNull()) {
        ctx.CreateSlot(
            TStringBuf("result"),
            TStringBuf("fact"),
            true,
            factSerpData
        );
    }
    return TResultValue();
}

void TFactsHandler::Register(THandlersMap* handlers) {
    auto factsForm = []() {
        return MakeHolder<TFactsHandler>();
    };
    handlers->emplace(FACTS_SCENARIO, factsForm);
}

} // namespace NBASS
