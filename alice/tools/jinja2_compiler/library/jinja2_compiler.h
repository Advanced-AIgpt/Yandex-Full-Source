#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

class TJinja2Compiler {
public:
    bool InitOptions(int argc, const char** argv);
    void PrintUsage();
    int Run();

    static constexpr TStringBuf VERSION = "0.0.1";

private:
    struct Options {
        struct InOut {
            TString InputTemplate;
            TString OutputFile;
        };

        TVector<TString> ProtoRoots;
        TVector<InOut> CompileList;
        TVector<TString> IncludeFolders;
        TVector<TString> ClassNames;
        TString DebugDump;
    };

    Options Options_;

};


} // namespace NAlice
