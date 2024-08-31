#include <alice/nlu/granet/lib/compiler/source_text_collection.h>
#include <alice/paskills/granet_server/config/proto/config.pb.h>

#include <library/cpp/json/json_writer.h>
#include <util/generic/fwd.h>
#include <util/generic/vector.h>

namespace NGranetServer {

struct TTestRunResult {
    TTestRunResult(TVector<TString> matched, TVector<TString> notMatched);
};

struct TGranetCompilerResult {

    const TString Base64Grammar;
    const TVector<TString> TruePositives;
    const TVector<TString> TrueNegatives;
    const TVector<TString> FalsePositives;
    const TVector<TString> FalseNegatives;

    TGranetCompilerResult(const TStringBuf base64Grammar,
                          TVector<TString>& truePositives,
                          TVector<TString>& trueNegatives,
                          TVector<TString>& falsePositives,
                          TVector<TString>& falseNegatives);

    NJson::TJsonValue ToJson() const;

};

TGranetCompilerResult CompileGrammar(const TWizardConfig& wizardConfig,
                                     const THashMap<TString, TString>& grammars,
                                     const TVector<TString>& expectedPositives,
                                     const TVector<TString>& expectedNegatives);

} // NGranetServer