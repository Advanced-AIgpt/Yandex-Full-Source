#pragma once

#include "data_loader.h"
#include "messages.h"
#include "src_line.h"
#include "source_text_collection.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

// ~~~~ TSourcesLoader ~~~~

class TSourcesLoader {
public:
    bool IsCompatibilityMode = false;
    ELanguage UILang = LANG_ENG;
    TVector<TFsPath> SourceDirs;
    IDataLoader* Loader = nullptr;
    TSourceTextCollection* CollectedSources = nullptr;

public:
    TString ReadFileAsString(const TTextView& source, const TFsPath& path);
    TSourceText::TConstRef ReadFileAsSourceText(const TTextView& source, const TFsPath& path);
};

// ~~~~ TLinesTreeBuilder ~~~~

class TLinesTreeBuilder {
public:
    explicit TLinesTreeBuilder(const TSourceText::TConstRef& sourceText);

    TSrcLine::TRef Build();

private:
    void CreateMaskedText();
    void CreateTree();
    size_t RemoveIndent(TTextView* line) const;
    void SplitByColon(TSrcLine* line) const;
    void SplitByKindOfLineBreaks(TSrcLine* line, char lineBreak) const;
    void UnmaskLines(TSrcLine* line) const;
    void UnmaskTextView(TTextView* view) const;
    void Check(bool condition, const TTextView& view, EMessageId messageId) const;
    void Check(bool condition, size_t offset, EMessageId messageId) const;

private:
    TSourceText::TConstRef OriginalText;
    TSourceText::TConstRef MaskedText;
    TSrcLine::TRef Root;
};

// ~~~~ TPreprocessor ~~~~

class TPreprocessor {
public:
    explicit TPreprocessor(const TSourcesLoader& sourcesLoader);

    TSrcLine::TRef PreprocessFile(const TTextView& source, const TFsPath& path);
    TSrcLine::TRef PreprocessString(const TString& text);

private:
    TSrcLine::TRef Preprocess(const TSourceText::TConstRef& sourceText);
    void ProcessDirectiveInclude(const TSrcLine::TRef& tree);

private:
    TSourcesLoader SourcesLoader;
};

} // namespace NGranet::NCompiler
