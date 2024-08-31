#include "granet.h"
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/parser/multi_parser.h>
#include <alice/nlu/granet/lib/test/fetcher.h>

namespace NGranet {

TGrammar::TRef CompileGrammarFromPath(const TFsPath& path, const TGranetDomain& domain,
    NCompiler::IDataLoader* loader)
{
    return NCompiler::TCompiler().CompileFromPath(path, domain, loader);
}

TGrammar::TRef CompileGrammarFromString(TStringBuf str, const TGranetDomain& domain) {
    return NCompiler::TCompiler().CompileFromString(str, domain);
}

TSample::TRef CreateSample(TStringBuf line, ELanguage language) {
    return TSample::Create(line, language);
}

void FetchEntities(const TSample::TRef& sample, const TGranetDomain& domain, const TBegemotFetcherOptions& options) {
    FetchSampleEntities(sample, domain, options);
}

TVector<TParserFormResult::TConstRef> ParseSample(const TGrammar::TConstRef& grammar, const TSample::TRef& sample) {
    return TMultiParser::Create(grammar, sample, true)->ParseForms();
}

} // namespace NGranet
