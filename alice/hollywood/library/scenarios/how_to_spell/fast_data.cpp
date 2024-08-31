#include "fast_data.h"

#include <util/charset/wide.h>

namespace NAlice::NHollywood {

namespace {

using TPhrasesWithReplies = google::protobuf::RepeatedPtrField<TPopularPhraseWithReply>;

THashMap<TUtf16String, TPhraseReply> ParsePhrasesWithReplies(const TPhrasesWithReplies& phrasesWithReplies) {
    THashMap<TUtf16String, TPhraseReply> result;

    for (const auto& phrase : phrasesWithReplies) {
        result[UTF8ToWide(phrase.GetPhrase())] = TPhraseReply{phrase.GetTextReply(), phrase.GetVoiceReply()};
    }

    return result;
}

THashMap<wchar16, TString> ParseSymbolsToPhonemes(const THowToSpellFastDataProto& proto) {
    THashMap<wchar16, TString> result;

    for (const auto& lettersWithPhonemes : proto.GetLettersWithPhonemes()) {
        for (wchar_t ch : UTF8ToWide(lettersWithPhonemes.GetLettersOptions())) {
            result[ch] = lettersWithPhonemes.GetPhoneme();
        }
    }

    return result;
}

THashMap<TString, TString> ParseAsrRecognitionRewriteData(const TAsrRecognitionRewriteData& data) {
    THashMap<TString, TString> result;

    for (const auto& item : data.GetAsrRecognitionRewriteItems()) {
        result[item.GetAsrText()] = item.GetRewrittenAsrText();
    }

    for (const auto& yoWord : data.GetWordsWithYoLetter()) {
        auto eWord = yoWord;
        SubstGlobal(eWord, "ё", "е");
        result[eWord] = yoWord;
    }

    return result;
}

}

THowToSpellFastData::THowToSpellFastData(const THowToSpellFastDataProto& proto)
    : PopularPhrasesToReplies(ParsePhrasesWithReplies(proto.GetPopularPhrasesWithReplies()))
    , LettersToReplies(ParsePhrasesWithReplies(proto.GetLettersWithReplies()))
    , RulesToReplies(ParsePhrasesWithReplies(proto.GetRulesWithReplies()))
    , SymbolsToPhonemes(ParseSymbolsToPhonemes(proto))
    , AsrRecognitionRewriteData(ParseAsrRecognitionRewriteData(proto.GetAsrRecognitionRewriteData()))
    , LettersVoiceSeparator(proto.GetLettersVoiceSeparator())
    , EnableVerificationWordsQueries(proto.GetEnableVerificationWordsQueries())
{
}

}  // namespace NAlice::NHollywood
