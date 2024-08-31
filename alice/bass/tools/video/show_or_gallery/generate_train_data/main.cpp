#include <alice/bass/libs/video_common/show_or_gallery/factors.h>
#include <alice/bass/libs/video_common/show_or_gallery/video.sc.h>
#include <alice/bass/libs/video_common/utils.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/join.h>
#include <util/system/yassert.h>

#include <cstdlib>

using namespace std;

using TItem = NVideoCommon::NShowOrGallery::TItem<TSchemeTraits>;
using TItems = TVector<TItem>;
using TFactors = NVideoCommon::NShowOrGallery::TFactors;

namespace {
TItems BuildItems(NSc::TArray& values) {
    auto onError = [](TStringBuf p, TStringBuf e) { Cerr << p << ": " << e << Endl; };

    TItems items;

    for (size_t i = 0; i < values.size(); ++i) {
        TItem item(&values[i]);

        if (!item.Validate(TString{} /* path */, false /* strict */, onError)) {
            Cerr << "Failed to validate:" << Endl;
            Cerr << values[i].ToJsonPretty() << Endl;
            exit(-1);
        }

        if (!item.Success() || item.Results().Empty())
            continue;

        items.emplace_back(item);
    }

    return items;
}

TVector<TFactors> BuildFactors(const TItems& items) {
    TVector<TFactors> fss;
    for (const auto& item : items) {
        TFactors fs;
        fs.FromItem(item);
        fss.push_back(fs);
    }
    return fss;
}

template <typename TValue>
void WriteLine(IOutputStream& os, const TVector<TValue>& values, TStringBuf sep) {
    bool first = true;

    for (const auto& value : values) {
        if (!first)
            os << sep;
        first = false;
        os << value;
    }
    os << Endl;
}

void WriteLine(IOutputStream& os, size_t id, const TItem& item, const TFactors& fs, TStringBuf sep) {
    NVideoCommon::TSepByLineEmitter emitter(os, sep);

    const auto topRelevantOnly = static_cast<int>(item.TopRelevantOnly());
    const auto& query = item.Query();

    emitter << id << topRelevantOnly << query.Text() << query.Freq();

    TVector<float> values;
    fs.GetValues(values);
    for (const auto& value : values)
        emitter << value;
}

void ToCSV(TStringBuf path, const TItems& items, const TVector<TFactors>& fss) {
    Y_ASSERT(items.size() == fss.size());

    TFileOutput out(TString{path});

    if (fss.empty())
        return;

    TVector<TString> names;
    names.emplace_back("id");
    names.emplace_back("top_relevant_only");
    names.emplace_back("query");
    names.emplace_back("weight");
    fss.front().GetNames(names);
    WriteLine(out, names, ",");

    for (size_t i = 0; i < fss.size(); ++i)
        WriteLine(out, i, items[i], fss[i], ",");
}

void ToTSV(TStringBuf path, const TItems& items, const TVector<TFactors>& fss) {
    TFileOutput out(TString{path});

    for (size_t i = 0; i < fss.size(); ++i)
        WriteLine(out, i, items[i], fss[i], "\t");
}
} // namespace

int main(int argc, char** argv) {
    TString outputCSV;
    TString outputTSV;

    auto opts = NLastGetopt::TOpts::Default();

    opts.AddLongOption("output-csv").StoreResult(&outputCSV).RequiredArgument("OUTPUT-CSV");
    opts.AddLongOption("output-tsv").StoreResult(&outputTSV).RequiredArgument("OUTPUT-TSV");
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

    const auto items = BuildItems(value.GetArrayMutable());
    const auto fss = BuildFactors(items);

    size_t numRelevant = 0;
    for (const auto& item : items) {
        if (item.TopRelevantOnly())
            ++numRelevant;
    }

    Cerr << "Num relevant: " << numRelevant << Endl;
    Cerr << "Num irrelevant: " << items.size() - numRelevant << Endl;

    if (!outputCSV.empty())
        ToCSV(outputCSV, items, fss);

    if (!outputTSV.empty())
        ToTSV(outputTSV, items, fss);

    return 0;
}
