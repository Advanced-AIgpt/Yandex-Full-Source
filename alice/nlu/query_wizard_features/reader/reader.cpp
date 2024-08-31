#include "reader.h"

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/xrange.h>
#include <util/memory/blob.h>
#include <util/string/split.h>

namespace NQueryWizardFeatures {

TReaderData::TReaderData(const TString& triePath, const TString& dataPath)
    : Trie(TBlob::FromFile(triePath))
    , DataReader(dataPath)
{
}

void TReader::Load(const TString& triePath, const TString& dataPath) {
    Data = MakeHolder<TReaderData>(triePath, dataPath);
}

TFeatures TReader::GetFeaturesForText(const TStringBuf& text) const {
    if (Data) {
        ui64 idx{};
        if (Data->Trie.Find(text, &idx)) {
            return GetFeaturesByIndex(idx, text);
        }
    }
    return {};
}

TVector<TFeaturesForFragment> TReader::GetFeaturesForTextFragments(const TStringBuf& text) const {
    if (!Data) {
        return {};
    }

    TVector<TStringBuf> words;
    Split(text, " ", words);
    TVector<TString> inflectedWords(words.size());

    // Assume that creating an inflector instance is cheap.
    NInfl::TSimpleInflector inflector("ru");
    for (size_t idx : xrange(words.size())) {
        const auto inflected = inflector.Inflect(UTF8ToWide(words[idx]), "nom");
        // The inflector may fail to inflect the string; fall back to the original
        // version of the word.
        inflectedWords[idx] = inflected.empty() ? words[idx] : WideToUTF8(inflected);
    }

    TVector<TFeaturesForFragment> result;

    for (size_t startWord = 0; startWord < words.size(); ++startWord) {
        size_t byteStart = words[startWord].data() - text.data();
        size_t charStart = GetNumberOfUTF8Chars(text.SubStr(0, byteStart));

        const auto partSuffix = text.SubStr(byteStart);

        TTrie::TPhraseMatchVector matches;
        Data->Trie.FindPhrases(partSuffix, matches);

        for (const auto& match : matches) {
            const auto fragment = partSuffix.SubStr(0, match.first);
            TFeaturesForFragment features;
            features.CharStart = charStart;
            features.Fragment = fragment;
            features.NormalizedFragment = fragment;
            features.Features = GetFeaturesByIndex(match.second, fragment);
            result.push_back(features);
        }

        TString combined;
        size_t fragmentLen = 0;
        bool hasSomethingToNormalize = false;
        for (size_t nextWord = startWord; nextWord < words.size() && nextWord < startWord + 5; ++nextWord) {
            if (inflectedWords[nextWord].empty()) {
                break;
            }
            if (words[nextWord] != inflectedWords[nextWord]) {
                hasSomethingToNormalize = true;
            }
            if (nextWord > startWord) {
                combined.push_back(' ');
                ++fragmentLen;
            }
            combined += inflectedWords[nextWord];
            fragmentLen += words[nextWord].size();
            if (hasSomethingToNormalize) {
                ui64 idx{};
                if (Data->Trie.Find(combined, &idx)) {
                    TFeaturesForFragment features;
                    features.CharStart = charStart;
                    features.Fragment = partSuffix.SubStr(0, fragmentLen);
                    features.NormalizedFragment = combined;
                    features.Features = GetFeaturesByIndex(idx, combined);
                    result.push_back(features);
                }
            }
        }
    }
    return result;
}


void TReader::Dump(IOutputStream& out) {
    for (const auto& rec : Data->Trie) {
        out << rec.first << '\t';
        try {
            const auto& features = GetFeaturesByIndex(rec.second, rec.first);
            out << features.AsJSON();
        } catch (...) {
            out << CurrentExceptionMessage();
        }
        out << '\n';
    }
}

TFeatures TReader::GetFeaturesByIndex(ui64 idx, const TStringBuf& text) const {
    TFeatures result;
    NSuggest::TBlobReader::TRec rec;
    Y_ENSURE(Data->DataReader.GetRecord(idx, &rec), "Failed to fetch protobuf data for " << text);
    Y_ENSURE(result.ParseFromArray(rec.Start, rec.End - rec.Start), "Failed to parse protobuf data for " << text);
    return result;
}

}

