#include "translit.h"

#include <dict/dictutil/gc.h>
#include <dict/misspell/prompter/lib/prompter.h>
#include <dict/libs/trie/trie.h>

#include <kernel/lemmer/core/language.h>
#include <library/cpp/resource/resource.h>

#include <util/charset/wide.h>
#include <util/stream/mem.h>

namespace {

class TPrompter {
public:
    TPrompter() : Instance(GetPrompter()) {
    }

    TVector<TWordSuggestion> MakeSuggestions(TStringBuf name) const {
        return Instance->MakeSuggestions(UTF8ToWide(name));
    }

private:
    std::unique_ptr<IWordPrompter> GetPrompter() {
        TString langModelData;
        TString transfemesData;

        if (!NResource::FindExact("names_lang_model", &langModelData))
            ythrow yexception() << TStringBuf("Can't find names_lang_model resource");

        if (!NResource::FindExact("ru-en.translit", &transfemesData))
            ythrow yexception() << TStringBuf("Can't find ru-en.translit resource");

        TMemoryInput langModelStream(langModelData.data(), langModelData.size());
        TMemoryInput transfemesStream(transfemesData.data(), transfemesData.size());

        return Create(transfemesStream, langModelStream);
    }

    std::unique_ptr<IWordPrompter> Create( TMemoryInput& transfemesStream, TMemoryInput& LmInput) {
        XTransfemePrompterParameters<wchar16> params;

        params.Trie = Gc.Push(NDict::NUtil::XTrie<wchar16>::FromStream(&LmInput));
        params.RulesInputStream = &transfemesStream;
        params.MaxSuggestions = 5;
        params.NodesPerBucketAtLength.clear();
        params.NodesPerBucket = 1000;
        params.BeamDistanceMultiplier = 2.;
        params.RankingDistanceMultiplier = 2.;

        return CreateTransfemePrompter(params);
    }

private:
    TGC Gc;
    std::unique_ptr<IWordPrompter> Instance;
};

} // anonymous namespace

namespace NBASS {

bool IsoTranslit(TStringBuf value, TString& translit) {
    THolder<TUntransliter> translitIterator = NLemmer::GetLanguageById(LANG_RUS)->GetTransliter();

    TUtf16String word = UTF8ToWide(value);
    translitIterator->Init(word);
    TUntransliter::WordPart res = translitIterator->GetNextAnswer();
    if (res.Empty())
        return false;

    translit = WideToUTF8(res.GetWord());
    return !translit.empty();
}

namespace NCall {

TVector<TString> FioTranslit(TStringBuf value) {
    static TPrompter prompter;

    TVector<TString> synonyms;
    TString isoValue;
    if (IsoTranslit(value, isoValue))
        synonyms.push_back(isoValue);

    for (const auto& sug : prompter.MakeSuggestions(value)) {
        synonyms.push_back(WideToUTF8(sug.Text));
    }

    return synonyms;
}

} // namespace NCall
} // namespace NBASS
