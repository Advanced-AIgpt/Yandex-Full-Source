#include "scenario.h"

#include <alice/hollywood/library/dispatcher/common_handles/util/util.h>

#include <util/system/backtrace.h>

namespace NAlice::NHollywood {

namespace {

void UpdateScenarioHandleSensors(TContext& context, const TScenario::THandleBase& handle,
                                 const ERequestResult requestResult, const i64 timeMs) {
    context.GlobalContext().Sensors().AddHistogram(ScenarioResponseTime(handle, requestResult), timeMs, NMetrics::TIME_INTERVALS);
    context.GlobalContext().Sensors().AddHistogram(ScenarioResponseTime(handle, ERequestResult::TOTAL), timeMs, NMetrics::TIME_INTERVALS);

    context.GlobalContext().Sensors().IncRate(ScenarioResponse(handle, requestResult));
    context.GlobalContext().Sensors().IncRate(ScenarioResponse(handle, ERequestResult::TOTAL));
    LOG_INFO(context.Logger()) << "Scenario " << handle.ScenarioName() << '/' << handle.Name() << " finished in " << timeMs
                               << " ms, result: " << requestResult;
}

} // anonymous namespace

void DispatchScenarioHandle(const TScenario& scenario,
                            const TScenario::THandleBase& handle,
                            TGlobalContext& globalContext,
                            NAppHost::IServiceContext& ctx,
                            const TScenarioNewContext* newContext /*= nullptr*/) {
    TInstant start = TInstant::Now();

    globalContext.Sensors().IncRate(ScenarioResponse(handle, ERequestResult::INCOMING));

    const auto appHostParams = GetAppHostParams(ctx);
    auto logger = CreateLogger(globalContext, GetRTLogToken(appHostParams, ctx.GetRUID()));
    const auto meta = GetMeta(ctx, logger);
    logger.UpdateRequestId(meta.GetRequestId(), ctx.GetLocation().Path);
    TContext context{globalContext, logger, scenario.Resources(), scenario.Nlg()};
    TRng rng{MultiHash(meta.GetRandomSeed(), handle.ScenarioName(), handle.Name())};

    const ELanguage lang = LanguageByName(meta.GetLang());
    Y_ENSURE(lang != ELanguage::LANG_UNK);

    const ELanguage userLang = LanguageByName(meta.GetUserLang() ? meta.GetUserLang() : meta.GetLang());
    Y_ENSURE(userLang != ELanguage::LANG_UNK);

    TScenarioHandleContext handleContext {
        ctx /* ServiceCtx */,
        meta /* RequestMeta */,
        context /* Ctx */,
        rng /* Rng */,
        lang /* Lang */,
        userLang /* UserLang */,
        appHostParams /* AppHostParams */,
        newContext /* NewContext */
    };

    // WARNING(a-square)! Meta may contain secrets that must not be logged
    // TODO(a-square): consider switching to protobuf attributes to mark sensitive data
    auto cleanMeta = meta;
    if (cleanMeta.GetOAuthToken()) {
        cleanMeta.SetOAuthToken("[censored]");
    }
    if (cleanMeta.GetUserTicket()) {
        cleanMeta.SetUserTicket("[censored]");
    }
    LOG_INFO(context.Logger()) << "Meta: "
                               << SerializeProtoText(std::move(cleanMeta), /* singleLineMode = */ true,
                                                     /* expandAny = */ false);

    try {
        handle.Do(handleContext);
        UpdateScenarioHandleSensors(context, handle, ERequestResult::SUCCESS, (TInstant::Now() - start).MilliSeconds());
    } catch (...) {
        UpdateScenarioHandleSensors(context, handle, ERequestResult::ERROR, (TInstant::Now() - start).MilliSeconds());
        LOG_ERROR(context.Logger()) << handle.ScenarioName() << ", " << handle.Name()
                                    << " has failed with exception, " << CurrentExceptionMessage() << "\n"
                                    << FormatCurrentException();
        throw;
    }
}

} // namespace NAlice::NHollywood
