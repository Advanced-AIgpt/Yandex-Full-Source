#pragma once

#include <alice/nlu/granet/lib/compiler/data_loader.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/parser/result.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <alice/nlu/granet/lib/test/fetcher.h>
#include <util/folder/path.h>

namespace NGranet {

// Compile grammar.
// path - path to grammar config.
// loader - if not null use this callback to load needed files (config, includes and dictionaries).
TGrammar::TRef CompileGrammarFromPath(const TFsPath& path, const TGranetDomain& domain,
    NCompiler::IDataLoader* loader = nullptr);

// Compile grammar from text.
TGrammar::TRef CompileGrammarFromString(TStringBuf str, const TGranetDomain& domain);

// Create sample (for parser).
TSample::TRef CreateSample(TStringBuf line, ELanguage language);

// Fetch sample entities from Begemot and save them into the sample.
// Can throw exception.
void FetchEntities(const TSample::TRef& sample, const TGranetDomain& domain,
    const TBegemotFetcherOptions& options = {});

// Parse sample by all forms stored in grammar.
TVector<TParserFormResult::TConstRef> ParseSample(const TGrammar::TConstRef& grammar, const TSample::TRef& sample);

} // namespace NGranet
