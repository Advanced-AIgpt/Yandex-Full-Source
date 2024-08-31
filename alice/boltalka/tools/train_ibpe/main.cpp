#include <library/cpp/containers/dense_hash/dense_hash.h>
#include <library/cpp/containers/heap_dict/heap_dict.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/hash.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>

struct TOptions {
    TString DatasetFilename;
    size_t NumBpeUnits;
    size_t AlphabetSize;
    TString AlphabetOutputFilename;
    TString MergesOutputFilename;
    bool SkipUnknown;

    TOptions(int argc, const char* argv[]) {
        NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
        opts
            .AddHelpOption();
        opts
            .AddLongOption('d', "dataset")
            .RequiredArgument("FILE")
            .Required()
            .StoreResult(&DatasetFilename)
            .Help("Path to dataset file with format: token_1[ token_2[ ...]][ \\t count]");
        opts
            .AddLongOption('n', "num-bpe-units")
            .RequiredArgument("INT")
            .Required()
            .StoreResult(&NumBpeUnits)
            .Help("Number of BPE units to train");
        opts
            .AddLongOption('a', "alphabet-size")
            .RequiredArgument("INT")
            .Required()
            .StoreResult(&AlphabetSize)
            .Help("This much most frequent tokens in dataset will be used as alphabet for BPE algorithm");
        opts
            .AddLongOption('v', "alphabet-output")
            .RequiredArgument("FILE")
            .Required()
            .StoreResult(&AlphabetOutputFilename)
            .Help("Output file for most frequent tokens in dataset");
        opts
            .AddLongOption('m', "merges-output")
            .RequiredArgument("FILE")
            .Required()
            .StoreResult(&MergesOutputFilename)
            .Help("Output file for BPE merges");
        opts
            .AddLongOption("skip-unknown")
            .NoArgument()
            .SetFlag(&SkipUnknown)
            .Help("Skip tokens out of most frequent ones. Otherwise unknown tokens will be assigned tokenId = alphabet-size + 1 (0-based)");

        opts.AddHelpOption('h');
        opts.SetFreeArgsNum(0);

        NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    }
};

void SplitTokensAndCount(TString* line, ui64* count) {
    *count = 1;
    size_t tabPos = line->find('\t');
    if (tabPos != TString::npos) {
        *count = FromString<ui64>(line->substr(tabPos + 1));
        line->resize(tabPos);
    }
}

using TStringToIdMap = TDenseHash<TString, size_t>;

TStringToIdMap CreateAlphabet(const TOptions& opts, size_t* datasetSize) {
    TFileInput in(opts.DatasetFilename);
    TDenseHash<TString, ui64> counts;
    ui64 totalCount = 0;
    *datasetSize = 0;
    for (TString line; in.ReadLine(line); ) {
        ui64 count;
        SplitTokensAndCount(&line, &count);
        for (auto split : StringSplitter(line).Split(' ')) {
            counts[TString{split.Token()}] += count;
            totalCount += count;
        }
        ++*datasetSize;
    }
    TVector<std::pair<ui64, TString>> countsVec;
    countsVec.reserve(counts.Size());
    for (const auto& pair : counts) {
        countsVec.emplace_back(pair.second, pair.first);
    }
    std::sort(countsVec.begin(), countsVec.end(), [](const auto& a, const auto& b) {
        return a.first > b.first || a.first == b.first && a.second < b.second;
    });

    ui64 unkCount = 0;
    for (size_t i = opts.AlphabetSize; i < countsVec.size(); ++i) {
        unkCount += countsVec[i].first;
    }
    Cerr << "Alphabet covers " << (totalCount - unkCount) * 100.0 / totalCount << " % of dataset." << Endl;

    if (countsVec.size() > opts.AlphabetSize) {
        countsVec.resize(opts.AlphabetSize);
    }
    TStringToIdMap alphabet;
    for (const auto& pair : countsVec) {
        const auto& token = pair.second;
        size_t tokenId = alphabet.Size();
        alphabet[token] = tokenId;
    }
    return alphabet;
}

void OutputAlphabet(const TOptions& opts, const TStringToIdMap& alphabet) {
    TVector<TString> result(alphabet.Size());
    for (const auto& token : alphabet) {
        result[token.second] = token.first;
    }
    TFixedBufferFileOutput out(opts.AlphabetOutputFilename);
    for (const auto& token : result) {
        out << token << '\n';
    }
}

void ParseDataset(const TOptions& opts, const TStringToIdMap& alphabet, TVector<TVector<ui32>>* lines, TVector<ui64>* counts) {
    const size_t eosId = alphabet.Size();
    const size_t unkId = alphabet.Size() + 1;

    TFileInput in(opts.DatasetFilename);
    for (TString line; in.ReadLine(line); ) {
        ui64 count;
        SplitTokensAndCount(&line, &count);
        lines->emplace_back();
        counts->push_back(count);
        for (auto split : StringSplitter(line).Split(' ')) {
            if (const auto* p = alphabet.FindPtr(TString{split.Token()})) {
                lines->back().push_back(*p);
            } else {
                if (opts.SkipUnknown) {
                    continue;
                }
                lines->back().push_back(unkId);
            }
        }
        if (lines->back().empty()) {
            lines->pop_back();
            counts->pop_back();
        } else {
            lines->back().push_back(eosId);
        }
    }
}

void TrainBpe(const TOptions& opts, const TStringToIdMap& alphabet, size_t datasetSize) {
    TVector<TVector<ui32>> lines;
    TVector<ui64> counts;
    lines.reserve(datasetSize);
    counts.reserve(datasetSize);
    Cerr << "Parsing dataset..." << Endl;
    ParseDataset(opts, alphabet, &lines, &counts);

    using TPair = std::pair<ui32, ui32>;
    struct TPairStat {
        ui64 Count;
        // ui32 is used for lower memory consumption
        TDenseHashSet<ui32, THash<ui32>, /*maxLoadFactor*/75, /*logInitSize*/0> SrcStrIds{/*emptyKey*/Max<ui32>()};

        bool operator<(const TPairStat& other) const {
            return Count < other.Count;
        }
    };
    using TPairStats = THeapDict<TPair, TPairStat>;

    Cerr << "Preparing stats..." << Endl;
    TPairStats pairStats;
    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        ui64 count = counts[i];
        for (size_t j = 0; j + 1 < line.size(); ++j) {
            TPair pair(line[j], line[j + 1]);
            auto& stat = pairStats[pair];
            stat.Count += count;
            stat.SrcStrIds.Insert(i);
        }
    }

    ui32 newTokenId = alphabet.Size() + /*eos*/1 + /*unk*/!opts.SkipUnknown;

    Cerr << "Training..." << Endl;
    TFixedBufferFileOutput out(opts.MergesOutputFilename);
    for (size_t iter = 0; iter < opts.NumBpeUnits; ++iter, ++newTokenId) {
        if (pairStats.empty()) {
            Cerr << "Did not manage to build " << opts.NumBpeUnits << " units!" << Endl;
            return;
        }

        const auto& best = pairStats.top();
        TPair bestPair = best.first;
        ui64 bestCount = best.second.Count;
        out << bestPair.first << '\t' << bestPair.second << '\t' << bestCount << '\n';

        auto srcStrIds = best.second.SrcStrIds;
        for (size_t strId : srcStrIds) {
            auto& line = lines[strId];
            ui64 lineCount = counts[strId];
            for (size_t i = line.size() - 1; i >= 1;) {
                TPair pair(line[i - 1], line[i]);
                if (pair == bestPair) {
                    if (i - 1) {
                        TPair left(line[i - 2], line[i - 1]);
                        auto it = pairStats.find(left);
                        it->second.Count -= lineCount;
                        if (it->second.Count == 0) {
                            pairStats.erase(it);
                        }
                    }
                    if (i + 1 < line.size()) {
                        TPair right(line[i], line[i + 1]);
                        auto it = pairStats.find(right);
                        it->second.Count -= lineCount;
                        if (it->second.Count == 0) {
                            pairStats.erase(it);
                        }
                    }
                    line[i - 1] = newTokenId;
                    line.erase(line.begin() + i);
                    if (i - 1) {
                        TPair left(line[i - 2], line[i - 1]);
                        auto& stat = pairStats[left];
                        stat.Count += lineCount;
                        stat.SrcStrIds.Insert(strId);
                    }
                    if (i < line.size()) {
                        TPair right(line[i - 1], line[i]);
                        auto& stat = pairStats[right];
                        stat.Count += lineCount;
                        stat.SrcStrIds.Insert(strId);
                    }
                    i -= Min<ui64>(i, 2);
                } else {
                    --i;
                }
            }
        }
        pairStats.erase(bestPair);

        Cerr << (iter + 1) * 100 / opts.NumBpeUnits << "% done\r";
    }
    Cerr << Endl;
}

int main(int argc, const char* argv[]) {
    TOptions opts(argc, argv);

    size_t datasetSize; // for preallocating
    Cerr << "Creating alphabet..." << Endl;
    auto alphabet = CreateAlphabet(opts, &datasetSize);
    OutputAlphabet(opts, alphabet);
    Cerr << "Training BPE..." << Endl;
    TrainBpe(opts, alphabet, datasetSize);

    return 0;
}
