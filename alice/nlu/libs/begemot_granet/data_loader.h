#pragma once

#include <alice/nlu/granet/lib/compiler/data_loader.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <search/begemot/core/filesystem.h>

namespace NGranet {

// ~~~~ TBegemotFsDataLoader ~~~~

class TBegemotFsDataLoader : public NCompiler::IDataLoader {
public:
    explicit TBegemotFsDataLoader(const NBg::TFileSystem& fs);

    void SetFullDumpOutput(IOutputStream* out);
    void SetBriefDumpOutput(IOutputStream* out);

    virtual bool IsFile(const TFsPath& path) override;
    virtual TString ReadTextFile(const TFsPath& path) override;

private:
    const NBg::TFileSystem& FileSystem;
    IOutputStream* FullDump = nullptr;
    IOutputStream* BriefDump = nullptr;
};

// ~~~~ TBegemotGrammars ~~~~

struct TBegemotGrammars {
    TVector<NGranet::TGrammar::TConstRef> Grammars;
    TVector<TString> InitializationErrors;
    TString DataFullDump;
    TString DataBriefDump;

    TString GetDataDump(bool isFull) const {
        return isFull ? DataFullDump : DataBriefDump;
    }
};

// ~~~~ Utils ~~~~

TBegemotGrammars CompileBegemotGrammars(const NBg::TFileSystem& fs, const TFsPath& dir, bool throwOnError);

TGrammar::TConstRef FindGrammarByDomain(const TVector<TGrammar::TConstRef>& grammars,
    const TGranetDomain& domain);

} // namespace NGranet
