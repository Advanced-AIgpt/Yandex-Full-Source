#include "data_loader.h"
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <util/stream/str.h>

namespace NGranet {

// ~~~~ TBegemotFsDataLoader ~~~~

TBegemotFsDataLoader::TBegemotFsDataLoader(const NBg::TFileSystem& fs)
    : FileSystem(fs)
{
}

void TBegemotFsDataLoader::SetFullDumpOutput(IOutputStream* out) {
    FullDump = out;
}

void TBegemotFsDataLoader::SetBriefDumpOutput(IOutputStream* out) {
    BriefDump = out;
}

bool TBegemotFsDataLoader::IsFile(const TFsPath& path) {
    return FileSystem.Exists(path);
}

TString TBegemotFsDataLoader::ReadTextFile(const TFsPath& path) {
    const TString text = FileSystem.LoadInputStream(path)->ReadAll();
    TRACE_LINE(FullDump, ">>>> " << path << ":");
    TRACE_LINE(FullDump, text);
    TRACE_LINE(BriefDump, "  " << path << ": " << StringSplitter(text).Split('\n').Count() << " lines");
    return text;
}

static const TFsPath GRAMMAR_MAIN_FILE = "main.grnt";

TBegemotGrammars CompileBegemotGrammars(const NBg::TFileSystem& fs, const TFsPath& dir, bool throwOnError) {
    TBegemotGrammars result;

    try {
        TStringOutput fullDumpOut(result.DataFullDump);
        TStringOutput briefDumpOut(result.DataBriefDump);

        TBegemotFsDataLoader loader(fs);
        loader.SetFullDumpOutput(&fullDumpOut);
        loader.SetBriefDumpOutput(&briefDumpOut);

        THolder<NBg::TFileSystem> dirFs = fs.Subdirectory(dir);
        Y_ENSURE(dirFs, "Error! Cannot find directory " << dir);

        for (const TString& domainName : dirFs->List()) {
            TGranetDomain domain;
            if (!domain.TryFromDirName(domainName)) {
                continue;
            }
            const TFsPath grammarPath = dir / domainName / GRAMMAR_MAIN_FILE;
            fullDumpOut << ">>>> Compile " << grammarPath << Endl;
            briefDumpOut << ">>>> Compile " << grammarPath << Endl;
            try {
                NCompiler::TCompiler compiler({.IsCompatibilityMode = true});
                result.Grammars.push_back(compiler.CompileFromPath(grammarPath, domain, &loader));
            } catch (const yexception& e) {
                if (throwOnError) {
                    throw;
                }
                result.InitializationErrors.push_back(TStringBuilder() << "Error while compiling " << grammarPath << ": " << e.what());
            }
        }
    } catch (const yexception& e) {
        if (throwOnError) {
            throw;
        }
        result.InitializationErrors.push_back(e.what());
    }

    return result;
}

TGrammar::TConstRef FindGrammarByDomain(const TVector<TGrammar::TConstRef>& grammars,
    const TGranetDomain& domain)
{
    for (const TGrammar::TConstRef& grammar : grammars) {
        if (grammar->GetDomain() == domain) {
            return grammar;
        }
    }
    return nullptr;
}

} // namespace NGranet
