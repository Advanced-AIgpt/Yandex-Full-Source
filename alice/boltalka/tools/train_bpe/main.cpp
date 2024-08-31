#include <library/cpp/containers/heap_dict/heap_dict.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

struct TTokenWithCount {
    TUtf16String Token;
    ui64 Count;
};

TVector<TTokenWithCount> LoadDict(const TString& fileName, wchar16 sentinel) {
    TFileInput fin(fileName);
    TVector<TTokenWithCount> dict;
    for (TUtf16String line; fin.ReadLine(line);) {
        ui64 tabPos = line.find('\t');
        TUtf16String token = line.substr(0, tabPos);
        token += sentinel;
        ui64 count = 1;
        if (tabPos != TUtf16String::npos) {
            count = FromString<ui64>(line.substr(tabPos + 1));
        }
        dict.push_back({token, count});
    }
    return dict;
}

void OutputAlphabet(const TVector<TTokenWithCount>& dict, ui64 alphabetSize) {
    THeapDict<wchar16, ui64> charCount;
    for (const auto& tokenWithCount : dict) {
        for (wchar16 ch : tokenWithCount.Token) {
            charCount[ch] += tokenWithCount.Count;
        }
    }
    for (ui64 iter = 0; iter < alphabetSize && !charCount.empty(); ++iter, charCount.pop()) {
        const auto& pair = charCount.top();
        Cout << TWtringBuf(&pair.first, 1) << '\t' << pair.second << '\n';
    }
    Cout << Flush;
}

void TrainBpeFast(const TVector<TTokenWithCount>& dict, ui64 numUnits) {
    TVector<TVector<TWtringBuf>> tokens(dict.size());
    for (ui64 i = 0; i < dict.size(); ++i) {
        const auto& token = dict[i].Token;
        tokens[i].resize(token.size());
        for (ui64 j = 0; j < token.size(); ++j) {
            tokens[i][j] = TWtringBuf(token.data() + j, 1);
        }
    }

    using TPair = std::pair<TWtringBuf, TWtringBuf>;
    struct TPairStat {
        ui64 Count;
        THashSet<ui64> SrcStrIds;
        bool operator<(const TPairStat& other) const {
            return Count < other.Count;
        }
    };
    using TPairStats = THeapDict<TPair, TPairStat>;

    TPairStats pairStats;
    for (ui64 i = 0; i < dict.size(); ++i) {
        const auto& token = tokens[i];
        ui64 count = dict[i].Count;
        for (ui64 j = 0; j + 1 < token.size(); ++j) {
            TPair pair(token[j], token[j + 1]);
            auto& stat = pairStats[pair];
            stat.Count += count;
            stat.SrcStrIds.insert(i);
        }
    }

    for (ui64 iter = 0; iter < numUnits; ++iter) {
        if (pairStats.empty()) {
            Cerr << "Did not manage to build " << numUnits << " units!" << Endl;
            return;
        }

        const auto& best = pairStats.top();
        TPair bestPair = best.first;
        ui64 bestCount = best.second.Count;
        Cout << bestPair.first << '\t' << bestPair.second << '\t' << bestCount << '\n';

        auto srcStrIds = best.second.SrcStrIds;
        for (ui64 strId : srcStrIds) {
            auto& token = tokens[strId];
            ui64 tokenCount = dict[strId].Count;
            for (ui64 i = token.size() - 1; i >= 1;) {
                TPair pair(token[i - 1], token[i]);
                if (pair == bestPair) {
                    if (i - 1) {
                        TPair left(token[i - 2], token[i - 1]);
                        auto it = pairStats.find(left);
                        it->second.Count -= tokenCount;
                        if (it->second.Count == 0) {
                            pairStats.erase(it);
                        }
                    }
                    if (i + 1 < token.size()) {
                        TPair right(token[i], token[i + 1]);
                        auto it = pairStats.find(right);
                        it->second.Count -= tokenCount;
                        if (it->second.Count == 0) {
                            pairStats.erase(it);
                        }
                    }
                    token[i - 1] = TWtringBuf(token[i - 1].data(), token[i - 1].size() + token[i].size());
                    token.erase(token.begin() + i);
                    if (i - 1) {
                        TPair left(token[i - 2], token[i - 1]);
                        auto& stat = pairStats[left];
                        stat.Count += tokenCount;
                        stat.SrcStrIds.insert(strId);
                    }
                    if (i < token.size()) {
                        TPair right(token[i - 1], token[i]);
                        auto& stat = pairStats[right];
                        stat.Count += tokenCount;
                        stat.SrcStrIds.insert(strId);
                    }
                    i -= Min<ui64>(i, 2);
                } else {
                    --i;
                }
            }
        }
        pairStats.erase(bestPair);
    }
}

void TrainBpeSlow(const TVector<TTokenWithCount>& dict, ui64 numUnits) {
    TVector<TVector<TUtf16String>> tokens(dict.size());
    for (ui64 i = 0; i < dict.size(); ++i) {
        const auto& token = dict[i].Token;
        for (wchar16 ch : token) {
            tokens[i].push_back(TUtf16String(1, ch));
        }
    }

    using TPair = std::pair<TUtf16String, TUtf16String>;
    using TPairCounts = THashMap<TPair, ui64>;

    for (ui64 iter = 0; iter < numUnits; ++iter) {
        TPairCounts pairCounts;
        for (ui64 i = 0; i < dict.size(); ++i) {
            const auto& token = tokens[i];
            ui64 count = dict[i].Count;
            for (ui64 j = 0; j + 1 < token.size(); ++j) {
                TPair pair(token[j], token[j + 1]);
                pairCounts[pair] += count;
            }
        }
        if (pairCounts.empty()) {
            Cerr << "Did not manage to build " << numUnits << " units!" << Endl;
            return;
        }
        TPair bestPair;
        ui64 maxCount = 0;
        for (const auto& kv : pairCounts) {
            const auto& pair = kv.first;
            ui64 count = kv.second;
            if (count > maxCount) {
                bestPair = pair;
                maxCount = count;
            }
        }
        Cout << bestPair.first << '\t' << bestPair.second << '\t' << maxCount << '\n';

        for (ui64 i = 0; i < dict.size(); ++i) {
            auto& token = tokens[i];
            for (int j = token.size() - 1; j >= 1;) {
                TPair pair(token[j - 1], token[j]);
                if (pair == bestPair) {
                    token[j - 1] += token[j];
                    token.erase(token.begin() + j);
                    j -= 2;
                } else {
                    --j;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('d', "dict")
        .Required()
        .Help("Path to dict file with format: token[ \\t count]\n\n\n");
    opts
        .AddLongOption('n', "num-bpe-units")
        .OptionalArgument()
        .DefaultValue("10000")
        .Help("Number of BPE units to train\n\n\n");
    opts
        .AddLongOption('s', "sentinel")
        .OptionalArgument()
        .DefaultValue("-")
        .Help("End of token symbol\n\n\n");
    opts
        .AddLongOption('a', "alphabet-size")
        .DefaultValue("0")
        .Help("Output at most this much most frequent symbols before BPE units\n\n\n");

    TOptsParseResult res(&opts, argc, argv);

    wchar16 sentinel = UTF8ToWide(res.Get("sentinel"))[0];
    Cerr << "Loading dict..." << Endl;
    auto dict = LoadDict(res.Get("dict"), sentinel);

    //Cerr << "Training BPE..." << Endl;
    //TrainBpeSlow(dict, res.Get<ui64>("num-bpe-units"));

    ui64 alphabetSize = res.Get<ui64>("alphabet-size");
    if (alphabetSize != 0) {
        Cerr << "Outputing alphabet..." << Endl;
        OutputAlphabet(dict, alphabetSize);
    }

    Cerr << "Training BPE..." << Endl;
    TrainBpeFast(dict, res.Get<ui64>("num-bpe-units"));

    return 0;
}
