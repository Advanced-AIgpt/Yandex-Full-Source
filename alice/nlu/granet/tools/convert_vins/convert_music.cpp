#include "convert_music.h"
#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/compiler/nlu_line.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/vins_converter/vins_config.h>
#include <alice/nlu/granet/lib/vins_converter/vins_converter.h>
#include <util/datetime/cputimer.h>
#include <util/string/subst.h>

namespace NGranet {

using namespace NCompiler;

namespace NPathes {
    static const TFsPath VinsDataDir = "alice/vins/apps/personal_assistant/personal_assistant";
    static const TFsPath VinsConfigFile = VinsDataDir / "config/Vinsfile.json";
    static const TFsPath MusicSearchTextFile = VinsDataDir / "config/nlu_templates/music/music_search_text.txt";
    static const TFsPath TaggerValidationDir = VinsDataDir / "tests/validation_sets/tagger_validation";
    static const TFsPath MusicValidationFile = TaggerValidationDir / "personal_assistant.scenarios.music_play.nlu";
    static const TFsPath FillersFile = "alice/nlu/granet/tools/convert_vins/data/fillers.txt";
}

TConvertMusicApplication::TConvertMusicApplication(const TFsPath& arcadiaDir, const TFsPath& resultsDir)
    : ArcadiaDir(arcadiaDir)
    , ResultsDir(resultsDir)
{
}

void TConvertMusicApplication::Process() {
    ResultsDir.MkDirs();

    Convert();

    Test();
}

void TConvertMusicApplication::Convert() {
    const NVinsConfig::TVinsConfig vinsConfig = NVinsConfig::ReadVinsConfig(ArcadiaDir / NPathes::VinsConfigFile);

    TVinsConverter converter(vinsConfig, "scenarios", "music_play", ResultsDir);
    converter.SetFillersDictionary("filler", ArcadiaDir / NPathes::FillersFile);
    converter.AddTagDictionary("search_text", ArcadiaDir / NPathes::MusicSearchTextFile);
    converter.Process();
}

void TConvertMusicApplication::Test() {
    TGrammar::TConstRef grammar = CompileGrammarFromPath(ResultsDir / "grammar.json", {});

    TFileOutput out(ResultsDir / "results.txt");

    double lineCount = 0;
    TVector<int> trueCount(10);
    int failNotEmpty = 0;
    int failEmpty = 0;

    TFileInput input(ArcadiaDir / NPathes::MusicValidationFile);
    TString validationLine;
    while (input.ReadLine(validationLine)) {
        TString line;
        TVector<TTag> validationTags;
        if (!TryReadTaggerMarkup(validationLine, &line, &validationTags)) {
            out << "Error! Can't parse line " << Cite(validationLine) << Endl;
            continue;
        }
        validationTags = NormalizeNames(validationTags);
        if (line.empty()) {
            continue;
        }
        lineCount++;

        TVector<TParserVariant::TConstRef> variants = Parse(grammar, line);

        size_t trueIndex = 0;
        for (;trueIndex < variants.size(); ++trueIndex) {
            if (NormalizeNames(ToTags(variants[trueIndex]->ToMarkup(), 0)) == validationTags) {
                break;
            }
        }
        const bool hasTrue = trueIndex != variants.size();
        if (hasTrue && trueIndex < trueCount.size()) {
            trueCount[trueIndex]++;
        }
        if (hasTrue && trueIndex == 0) {
            // Success
            continue;
        }

        out << "- val: " << validationLine << Endl;
        if (!variants.empty()) {
            const TParserVariant::TConstRef& top = variants.front();
            out << "  res: " << PrintTaggerMarkup(line, NormalizeNames(ToTags(top->ToMarkup(), 0)));
            out << " : logprob " << top->LogProb << Endl;
        }
        out << LeftPad("Fail. True hypothesis index: ", 80);
        if (hasTrue) {
            out << trueIndex << " of " << variants.size();
            out << ". logprob: " << variants[trueIndex]->LogProb << Endl;
        } else {
            out << "none of " << variants.size() << Endl;
            if (!variants.empty()) {
                failNotEmpty++;
            } else {
                failEmpty++;
            }
        }
    }
    for (size_t i = 1; i < trueCount.size(); ++i) {
        trueCount[i] += trueCount[i - 1];
    }
    out << Endl;
    out << "line count: " << lineCount << Endl;
    if (lineCount == 0) {
        return;
    }
    for (size_t i = 0; i < trueCount.size(); ++i) {
        out << "accuracy top " << i + 1 << ": " << trueCount[i] / lineCount << Endl;
    }
    out << "None of hypotheses is true:" << Endl;
    out << "  If hypotheses list not empty: " << failNotEmpty / lineCount << Endl;
    out << "  If hypotheses list empty: " << failEmpty / lineCount << Endl;
}

TVector<TParserVariant::TConstRef> TConvertMusicApplication::Parse(TGrammar::TConstRef grammar,
    const TString& line) const
{
    const TProfileTimer timer;

    TSample::TRef sample = CreateSample(line, LANG_RUS);
    TVector<TParserFormResult::TConstRef> forms = ParseSample(grammar, sample);

    if (timer.Get().MilliSeconds() > 100) {
        Cerr << timer.Get() << " in: " << line << Endl;
    }
    Y_VERIFY(forms.size() == 1);
    return forms.front()->GetVariants();
}

// static
TVector<TTag> TConvertMusicApplication::NormalizeNames(TVector<TTag> tags) {
    for (TTag& tag : tags) {
        TStringBuf name = tag.Name;
        name.SkipPrefix("+");
        name.ChopSuffix(".partial");
        tag.Name = name;
    }
    return tags;
}

} // namespace NGranet
