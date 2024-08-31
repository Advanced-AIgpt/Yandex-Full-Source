#include "multilang_facts.h"

#include <alice/protos/data/language/language.pb.h>
#include <alice/megamind/protos/scenarios/features/search.pb.h>

#include <alice/library/json/json.h>

#include <quality/functionality/facts/multilanguage_facts/processor/proto/io.pb.h>

namespace NAlice::NHollywood::NSearch {

namespace {

const TString ONTOFACTS_SCENARIO = "ontofacts";

constexpr TStringBuf EXP_ENABLE_MULTILANG_FACTS_SEARCH_FRAME = "hw_enable_multilang_facts_search_frame";

} // namespace


bool TMultilangFactsScenario::TryPrepare(NAppHost::IServiceContext& serviceCtx) {
    if (!IsEnabled()) {
        return false;
    }

    NFacts::NFactProcessor::TInput request;

    if (Ctx.GetRequest().HasExpFlag(EXP_ENABLE_MULTILANG_FACTS_SEARCH_FRAME) && Ctx.GetTaggerQuery()) {
        request.SetQuery(Ctx.GetTaggerQuery());
    } else {
        request.SetQuery(Ctx.GetRequest().Input().Utterance());
    }

    const auto lang = Ctx.GetRequest().BaseRequestProto().GetUserLanguage();
    request.SetLanguage(lang);
    LOG_INFO(Ctx.GetLogger()) << "Request facts service";
    LOG_INFO(Ctx.GetLogger()) << "Request proto: " << JsonFromProto(request);
    serviceCtx.AddProtobufItem(request, "multilang_fact_processor_input");
    Ctx.ReturnIrrelevant(true); // In case of render node fail to answer
    return true;
}

bool TMultilangFactsScenario::TryRender(const NAppHost::IServiceContext& serviceCtx) {
    if (!IsEnabled()) {
        return false;
    }
    if (!serviceCtx.HasProtobufItem("multilang_fact_processor_output")) {
        Ctx.ReturnIrrelevant(true);
        LOG_INFO(Ctx.GetLogger()) << "No multilang_facts response in context";
        return true;
    }

    const auto factsResponse = GetOnlyProtoOrThrow<NFacts::NFactProcessor::TOutput>(serviceCtx, "multilang_fact_processor_output");

    if (factsResponse.GetFailed()) {
        Ctx.ReturnIrrelevant(true);
        LOG_INFO(Ctx.GetLogger()) << "Failed to find relevant multilang fact";
        return true;
    }

    NJson::TJsonValue factoid;
    factoid["tts"] = factsResponse.GetText();
    factoid["text"] = factsResponse.GetText();
    factoid["url"] = factsResponse.GetUrl();
    factoid["title"] = factsResponse.GetHeadline();

    Ctx.GetFeatures().SetFoundSuggestFact(1);
    Ctx.AddRenderedCard(factoid);
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ONTOFACTS_SCENARIO);
    return true;
}

bool TMultilangFactsScenario::IsEnabled() {
    return Ctx.GetRequest().BaseRequestProto().GetUserLanguage() != ELang::L_RUS;
}

} // namespace NAlice::NHollywood::NSearch
