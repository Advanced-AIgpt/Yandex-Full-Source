#pragma once

#include <alice/nlu/granet/lib/sample/sample.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/parser/parser.h>
#include <util/folder/path.h>

namespace NGranet {

class TConvertMusicApplication {
public:
    TConvertMusicApplication(const TFsPath& arcadiaDir, const TFsPath& resultsDir);

    void Process();

private:
    void Convert();
    void Test();
    TVector<TParserVariant::TConstRef> Parse(TGrammar::TConstRef grammar, const TString& line) const;
    static TVector<TTag> NormalizeNames(TVector<TTag> tags);
    static bool MatchTags(const TVector<TTag>& resultTags, const TVector<TTag>& trueTags);
    static bool MatchTags(const TTag& resultTag, const TTag& trueTag);

private:
    TFsPath ArcadiaDir;
    TFsPath ResultsDir;
};

} // namespace NGranet
