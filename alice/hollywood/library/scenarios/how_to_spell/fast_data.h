#pragma once

#include <alice/hollywood/library/scenarios/how_to_spell/proto/fast_data.pb.h>

#include <alice/hollywood/library/fast_data/fast_data.h>

#include <util/generic/hash.h>


namespace NAlice::NHollywood {

struct TPhraseReply {
    TString TextReply;
    TString VoiceReply;
};

struct THowToSpellFastData : public IFastData {
    THowToSpellFastData(const THowToSpellFastDataProto& proto);

    const THashMap<TUtf16String, TPhraseReply> PopularPhrasesToReplies;
    const THashMap<TUtf16String, TPhraseReply> LettersToReplies;
    const THashMap<TUtf16String, TPhraseReply> RulesToReplies;
    const THashMap<wchar16, TString> SymbolsToPhonemes;
    const THashMap<TString, TString> AsrRecognitionRewriteData;
    const TString LettersVoiceSeparator;

    const bool EnableVerificationWordsQueries;
};

}  // namespace NAlice::NHollywood
