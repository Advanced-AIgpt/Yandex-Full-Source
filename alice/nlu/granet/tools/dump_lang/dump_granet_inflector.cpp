#include "dump_granet_inflector.h"
#include <alice/nlu/granet/lib/lang/inflector.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <kernel/lemmer/dictlib/terminals.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <util/string/split.h>

static void DumpSingle(TWtringBuf phrase, ELanguage lang, const TGramBitSet& destGrams,
    bool isVerbose, IOutputStream* log)
{
    const TUtf16String result = NGranet::InflectPhrase(phrase, lang, destGrams);
    if (isVerbose) {
        *log << NGranet::LeftJustify(result, 19) << " # " << destGrams.ToString() << Endl;
    } else {
        *log << result << Endl;
    }
}

static void DumpMulti(TWtringBuf phrase, ELanguage lang, TGramBitSet collected, TStringBuf expr,
    bool isVerbose, IOutputStream* log)
{
    if (expr.empty()) {
        DumpSingle(phrase, lang, collected, isVerbose, log);
        return;
    }
    TStringBuf mulPart = expr.NextTok('*');
    if (mulPart == "cases") {
        mulPart = "nom|gen|dat|acc|ins|abl|loc";
    }
    if (mulPart == "genders") {
        mulPart = "f|m|n|mf";
    }
    if (mulPart == "numbers") {
        mulPart = "sg|pl";
    }
    if (mulPart == "verbs") {
        mulPart = "inf|pf|ipf|pl,pf|pl,ipf";
    }
    for (TStringBuf orPart : StringSplitter(mulPart).Split('|')) {
        const TGramBitSet current = collected | TGramBitSet::FromString(orPart);
        DumpMulti(phrase, lang, current, expr, isVerbose, log);
    }
}

void DumpGranetInflector(TWtringBuf phrase, const TLangMask& langs, TStringBuf grams, bool isVerbose,
    IOutputStream* log)
{
    for (const ELanguage lang : langs) {
        DumpMulti(phrase, lang, {}, grams, isVerbose, log);
    }
}
