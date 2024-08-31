#pragma once

#include <alice/nlu/query_wizard_features/proto/features.pb.h>

#include <quality/trailer/suggest/data_structs/blob_reader.h>

#include <kernel/inflectorlib/phrase/simple/simple.h>
#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NQueryWizardFeatures {

using TTrie = TCompactTrie<char>;

struct TReaderData {
    TTrie Trie;
    NSuggest::TBlobReader DataReader;

    TReaderData(const TString& triePath, const TString& dataPath);
};

struct TFeaturesForFragment {
    size_t CharStart;
    TStringBuf Fragment;
    // The features are computed for this string. It may be equal to Fragment.
    TString NormalizedFragment;
    TFeatures Features;
};

class TReader {
public:
    void Load(const TString& triePath, const TString& dataPath);
    TFeatures GetFeaturesForText(const TStringBuf& text) const;

    // Find all subphrases of a text (possibly normalizing them along the way) that form
    // a popular search query and compute the corresponding features.
    TVector<TFeaturesForFragment> GetFeaturesForTextFragments(const TStringBuf& text) const;
    void Dump(IOutputStream& out);

private:
    TFeatures GetFeaturesByIndex(ui64 idx, const TStringBuf& text) const;

    THolder<TReaderData> Data;
};

}

