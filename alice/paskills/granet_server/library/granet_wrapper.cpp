#include "granet_wrapper.h"

#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/compiler/compiler.h>

#include <kernel/searchlog/errorlog.h>
#include <library/cpp/langs/langs.h>

#include <util/string/builder.h>
#include <util/stream/output.h>

namespace NGranetServer {

namespace {

    constexpr TStringBuf GrammarMainPath = "main.grnt";

    bool RunSingleTest(
        const NGranetServer::TWizardConfig& wizardConfig,
        const NGranet::TGrammar::TRef grammar,
        const TStringBuf test
    ) {
        NGranet::TSample::TRef sample = NGranet::CreateSample(test, ELanguage::LANG_RUS);
        NGranet::FetchEntities(sample, {.Lang = ELanguage::LANG_RUS, .IsPASkills = true}, {.Url = wizardConfig.GetUrl()});
        TVector<NGranet::TParserFormResult::TConstRef> forms = NGranet::ParseSample(grammar, sample);
        for (const NGranet::TParserFormResult::TConstRef& form : forms) {
            if (form->IsPositive()) {
                return true;
            }
        }
        return false;
    }

    void RunTests(
        const NGranetServer::TWizardConfig& wizardConfig,
        NGranet::TGrammar::TRef grammar,
        const TVector<TString>& tests,
        TVector<TString>& matched,
        TVector<TString>& notMatched
    ) {
        for (const auto& line: tests) {
            if (RunSingleTest(wizardConfig, grammar, line)) {
                matched.emplace_back(line);
            } else {
                notMatched.emplace_back(line);
            }
        }
    }

    NJson::TJsonValue VectorToJsonArray(const TVector<TString> elements) {
        NJson::TJsonValue jsonArray(NJson::JSON_ARRAY);
        for (const auto& element: elements) {
            jsonArray.AppendValue(NJson::TJsonValue(element));
        }
        return jsonArray;
    }

    /**
     * Generates fake main.grnt grammar that imports all user-provided grammars
     * TODO: generate random name if grammars already contains "main.grnt" grammar
     */
    TString GenerateMainGrnt(const THashMap<TString, TString>& grammars) {
        TStringBuilder builder;
        if (grammars.size() > 0) {
            constexpr TStringBuf indent = "    ";
            builder << "import:" << Endl;
            for (const auto& pair: grammars) {
                builder << indent << pair.first << Endl;
            }
        }
        return builder;
    }

}

TGranetCompilerResult::TGranetCompilerResult(
        const TStringBuf base64Grammar,
        TVector<TString>& truePositives,
        TVector<TString>& trueNegatives,
        TVector<TString>& falsePositives,
        TVector<TString>& falseNegatives):
    Base64Grammar(base64Grammar)
    , TruePositives(truePositives)
    , TrueNegatives(trueNegatives)
    , FalsePositives(falsePositives)
    , FalseNegatives(falseNegatives) {
}

NJson::TJsonValue TGranetCompilerResult::ToJson() const {
    NJson::TJsonValue json(NJson::JSON_MAP);
    json["grammar_base64"] = Base64Grammar;
    json["true_positives"] = VectorToJsonArray(TruePositives);
    json["true_negatives"] = VectorToJsonArray(TrueNegatives);
    json["false_positives"] = VectorToJsonArray(FalsePositives);
    json["false_negatives"] = VectorToJsonArray(FalseNegatives);
    return json;
}

TGranetCompilerResult CompileGrammar(
        const NGranetServer::TWizardConfig& wizardConfig,
        const THashMap<TString, TString>& grammars,
        const TVector<TString>& expectedPositives,
        const TVector<TString>& expectedNegatives
) {
    TVector<TString> truePositives, trueNegatives, falsePositives, falseNegatives;
    NGranet::NCompiler::TSourceTextCollection sourceCollection;
    sourceCollection.Domain.Lang = ELanguage::LANG_RUS;
    sourceCollection.Domain.IsPASkills = true;
    sourceCollection.Texts = grammars;
    sourceCollection.Texts[GrammarMainPath] = GenerateMainGrnt(grammars);
    sourceCollection.MainTextPath = GrammarMainPath;
    NGranet::TGrammar::TRef grammar = NGranet::NCompiler::TCompiler({ .UILang = LANG_RUS }).CompileFromSourceTextCollection(sourceCollection);
    RunTests(wizardConfig, grammar, expectedPositives, truePositives, falseNegatives);
    RunTests(wizardConfig, grammar, expectedNegatives, falsePositives, trueNegatives);
    TString base64Grammar = sourceCollection.ToCompressedBase64();
    return TGranetCompilerResult(base64Grammar, truePositives, trueNegatives, falsePositives, falseNegatives);
}

} // NGranetServer

