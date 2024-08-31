#include <alice/bass/libs/video_common/has_good_result/factors.h>
#include <alice/bass/libs/video_common/has_good_result/video.sc.h>
#include <alice/bass/libs/video_common/utils.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/output.h>

#include <cstdlib>

using TItem = NVideoCommon::NHasGoodResult::TItem<TSchemeTraits>;
using TItems = TVector<TItem>;

using TFactors = NVideoCommon::NHasGoodResult::TFactors;

namespace {
constexpr double EPS = 1e-6;
constexpr int WEIGHT = 1;

bool IsEmpty(const NVideoCommon::NHasGoodResult::TResultConst<TSchemeTraits>& result) {
    return result.Relevance() < EPS && result.RelevancePrediction() < EPS;
}

bool HasGoodResult(const TItem& item) {
    for (const auto& result : item.Results()) {
        if (result.Result() != "irrel")
            return true;
    }
    return false;
}

TItems BuildItems(NSc::TArray& values, bool skipEmpty) {
    auto onError = [](TStringBuf p, TStringBuf e) { Cerr << p << ": " << e << Endl; };

    TItems items;

    for (size_t i = 0; i < values.size(); ++i) {
        TItem item(&values[i]);

        if (!item.Validate(TString{} /* path */, false /* strict */, onError)) {
            Cerr << "Failed to validate:" << Endl;
            Cerr << values[i].ToJsonPretty() << Endl;
            exit(-1);
        }

        if (item.Results().Empty())
            continue;

        if (skipEmpty) {
            bool allEmpty = true;
            for (const auto& result : item.Results()) {
                if (!IsEmpty(result)) {
                    allEmpty = false;
                    break;
                }
            }

            if (allEmpty)
                continue;
        }

        items.emplace_back(item);
    }

    return items;
}

TVector<TFactors> BuildFactors(const TVector<TItem>& items) {
    TVector<TFactors> fss;

    for (const auto& item : items) {
        TFactors fs;
        fs.FromItem(item);
        fss.emplace_back(fs);
    }

    return fss;
}

void ToCSV(const TVector<TItem>& items, const TVector<TFactors>& fss, const TString& path, TStringBuf sep,
           bool displayHeader) {
    Y_ASSERT(items.size() == fss.size());

    TFileOutput os(path);

    if (displayHeader) {
        TVector<TString> names = {{"id", "target", "query", "weight"}};
        TFactors::GetNames(names);

        NVideoCommon::TSepByLineEmitter emitter(os, sep);
        for (const auto& name : names)
            emitter << name;
    }

    for (size_t id = 0; id < items.size(); ++id) {
        const auto& item = items[id];
        const auto& fs = fss[id];

        NVideoCommon::TSepByLineEmitter emitter(os, sep);

        emitter << id;
        emitter << static_cast<int>(HasGoodResult(item));
        emitter << item.Query();
        emitter << WEIGHT;

        TVector<float> values;
        fs.GetValues(values);
        for (const auto& value : values)
            emitter << value;
    }
}
} // namespace

int main(int argc, char** argv) {
    TString outputCSV;
    TString outputTSV;
    bool keepEmpty = false;

    auto opts = NLastGetopt::TOpts::Default();

    opts.AddLongOption("output-csv").StoreResult(&outputCSV).RequiredArgument("OUTPUT-CSV");
    opts.AddLongOption("output-tsv").StoreResult(&outputTSV).RequiredArgument("OUTPUT-TSV");
    opts.AddLongOption("keep-empty").SetFlag(&keepEmpty).NoArgument();
    opts.AddHelpOption();
    NLastGetopt::TOptsParseResult r(&opts, argc, argv);

    const TString json = Cin.ReadAll();

    NSc::TValue value;
    if (!NSc::TValue::FromJson(value, json)) {
        Cerr << "Failed to parse json" << Endl;
        return -1;
    }

    if (!value.IsArray()) {
        Cerr << "Input json is not an array" << Endl;
        return -1;
    }

    const auto items = BuildItems(value.GetArrayMutable(), !keepEmpty /* skipEmpty */);
    const auto fss = BuildFactors(items);

    size_t hasGoodResult = 0;
    for (const auto& item : items) {
        if (HasGoodResult(item))
            ++hasGoodResult;
    }

    Cout << "Number of items: " << items.size() << Endl;
    Cout << "Has good result: " << hasGoodResult << Endl;

    if (!outputCSV.empty())
        ToCSV(items, fss, outputCSV, "," /* sep */, true /* displayHeader */);

    if (!outputTSV.empty())
        ToCSV(items, fss, outputTSV, "\t" /* sep */, false /* displayHeader */);
    return 0;
}
