#include <alice/nlg/tools/render_context_tool/proto/config.pb.h>

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/library/util/rng.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/generic/set.h>


namespace {

const auto MEGAMIND_2_HOLLYWOOD_SUPPORTED_SCENARIO = THashMap<TString, TString>{
    {"Alarm", "alarm"},
    {"Weather", "weather"},
    {"Commands", "fast_command"},
    {"HollywoodMusic", "music"},
    {"HollywoodHardcodedMusic", "music"},
    {"News", "news"},
    {"Search", "search"},
};

void WritePhrase(IOutputStream& file, const NAlice::NNlg::TRenderPhraseResult& renderedPhrase, const TString& nlgRenderId,
                 const TString& scenarioName, const TString& templateName, const TString& phraseName, bool debug) {
    TStringStream stream;

    NJson::TJsonWriter json(&stream, /* formatOutput = */ false);
    json.OpenMap();
    json.Write("text", renderedPhrase.Text);
    json.Write("voice", renderedPhrase.Voice);
    json.Write("nlg_render_id", nlgRenderId);
    if (debug) {
        json.Write("scenario_name", scenarioName);
        json.Write("template_name", templateName);
        json.Write("phrase_name", phraseName);
    }
    json.CloseMap();

    json.Flush();

    file << stream.Str() << "\n";
}


void WriteCard(IOutputStream& file, const NAlice::NNlg::TRenderCardResult& renderedCard, const TString& nlgRenderId,
               const TString& scenarioName, const TString& templateName, const TString& phraseName, bool debug) {
    TStringStream stream;

    NJson::TJsonValue root(NJson::JSON_MAP);
    root["card"] = renderedCard.Card;
    root["nlg_render_id"] = nlgRenderId;
    if (debug) {
        root["scenario_name"] = scenarioName;
        root["template_name"] = templateName;
        root["phrase_name"] = phraseName;
    }

    NJson::WriteJson(&stream, &root, /* formatOutput = */ false);
    file << stream.Str() << "\n";
}


struct TContext {

    TContext(const TStringBuf jsonValue) {
        const auto json = NJson::ReadJsonFastTree(jsonValue);

        PhraseName = json["phrase_name"].GetString();
        CardName = json["card_name"].GetString();

        Y_ENSURE(PhraseName.empty() ^ CardName.empty());

        NlgRenderId = json["nlg_render_id"].GetStringSafe();
        const auto scenarioNme = json["scenario_name"].GetStringSafe();
        Y_ENSURE(MEGAMIND_2_HOLLYWOOD_SUPPORTED_SCENARIO.contains(scenarioNme), "Unknown scenario: " << scenarioNme << ", [NlgRenderId] = " << NlgRenderId);
        ScenarioName = MEGAMIND_2_HOLLYWOOD_SUPPORTED_SCENARIO.at(scenarioNme);
        TemplateName = json["template_name"].GetStringSafe();
        Language = json["language"].GetStringSafe();

        Context = json["context"];
        ReqInfo = json["req_info"];
        Form = json["form"];
    }

    TString NlgRenderId;
    TString ScenarioName;
    TString TemplateName;
    TString Language;
    TString PhraseName;
    TString CardName;
    NJson::TJsonValue Context;
    NJson::TJsonValue ReqInfo;
    NJson::TJsonValue Form;
};


void RenderMain(const TConfig& config) {
    NAlice::TRng rng{5};

    TSet<TString> scenarios;
    for (const auto& [_, scenario]: MEGAMIND_2_HOLLYWOOD_SUPPORTED_SCENARIO) {
        scenarios.insert(scenario);
    }

    NAlice::NHollywood::TScenarioRegistry& registry = NAlice::NHollywood::TScenarioRegistry::Get();
    registry.CreateScenarios(scenarios, config.GetScenarioResourcesPath(), nullptr, false);

    TString line;
    TFileInput file(config.input());

    TFileOutput out_phrases(config.output_phrases());
    TFileOutput out_cards(config.output_cards());

    int successPhrasesCnt = 0;
    int successCardsCnt = 0;
    int failedCnt = 0;

    while (file.ReadLine(line)) {
        if (line.empty()) {
            continue;
        }

        try {
            TContext ctx(line);

            try {
                auto langProto = config.language();
                if (langProto == ::NAlice::ELang::L_UNK) {
                    Y_ENSURE(::NAlice::ELang_Parse(ctx.Language, &langProto));
                }
                Y_ENSURE(::NAlice::ELang_IsValid(langProto) && langProto != ::NAlice::ELang::L_UNK);
                const auto lang = static_cast<ELanguage>(langProto);

                auto nlgData = NAlice::NHollywood::TNlgData(NAlice::TRTLogger::NullLogger());
                nlgData.Context = ctx.Context;
                nlgData.Form = ctx.Form;
                nlgData.ReqInfo = ctx.ReqInfo;

                if (!ctx.PhraseName.Empty()) {
                    const auto result = registry.GetScenario(ctx.ScenarioName).Nlg()->RenderPhrase(
                        ctx.TemplateName, ctx.PhraseName, lang, rng, nlgData
                    );
                    WritePhrase(out_phrases, result, ctx.NlgRenderId, ctx.ScenarioName, ctx.TemplateName, ctx.PhraseName, config.debug());
                    successPhrasesCnt++;
                } else {
                    const auto result = registry.GetScenario(ctx.ScenarioName).Nlg()->RenderCard(
                        ctx.TemplateName, ctx.CardName, lang, rng, nlgData
                    );
                    WriteCard(out_cards, result, ctx.NlgRenderId, ctx.ScenarioName, ctx.TemplateName, ctx.PhraseName, config.debug());
                    successCardsCnt++;
                }

            } catch (...) {
                Cerr << "Can't render phrase: [NlgRenderId] = " << ctx.NlgRenderId
                     << "[ScenarioName] = " << ctx.ScenarioName << ", [TemplateName] = " << ctx.TemplateName
                     << ", [PhraseName] = " << ctx.PhraseName << ", [CardName] = " << ctx.CardName << ", [Language] = " << ctx.Language << Endl;
                Cerr << CurrentExceptionMessage() << Endl;
                failedCnt++;
            }
        } catch (const yexception& ex) {
            Cerr << ex.what() << Endl;
            failedCnt++;
        }
    }
    out_phrases.Flush();
    out_cards.Flush();

    Cerr << "Successfully rendered " << successPhrasesCnt << " phrases and " << successCardsCnt << " cards, failed " << failedCnt << ".\n";
}

}  // namespace


int main(int argc, const char* argv[]) {
    try {
        TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);
        RenderMain(config);
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
