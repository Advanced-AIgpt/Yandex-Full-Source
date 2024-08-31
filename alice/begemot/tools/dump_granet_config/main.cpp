#include <alice/begemot/lib/utils/granet_config.h>
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/grammar/multi_grammar.h>
#include <alice/library/proto/proto.h>
#include <util/stream/output.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>


void doMain(int argc, const char* argv[]) {
    Y_ENSURE(argc > 2, "Usage: " << argv[0] << " GRANET_DOMAIN GRAMMAR_PATH");

    const TString domainString = argv[1];
    const TString grammarPath = argv[2];
    NGranet::TGranetDomain domain;
    Y_ENSURE(domain.TryFromDirName(domainString), "Bad domain: " << domainString);
    NGranet::TGrammar::TConstRef staticGrammar = NGranet::NCompiler::TCompiler().CompileFromPath(grammarPath, domain);
    NGranet::TMultiGrammar::TConstRef grammar = NGranet::TMultiGrammar::CreateForBegemot(
        staticGrammar,
        /* freshGrammar= */ {},
        /* externalGrammars= */ {},
        /* freshOptions= */ {},
        /* experiments= */ {},
        /* enabledConditionalTasks= */ {}
    );
    Cout << NAlice::SerializeProtoText(NBg::MakeGranetConfig(grammar), /* singleLineMode= */ false);
}


int main(int argc, const char* argv[]) {
    try {
        doMain(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << '\n';
        return 1;
    }
    return 0;
}

