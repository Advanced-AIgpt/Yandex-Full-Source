#include "how_to_spell.h"

#include "fast_data.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <util/charset/wide.h>
#include <util/string/cast.h>
#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

class THowToSpellScenarioEngine {
public:
    THowToSpellScenarioEngine(TScenarioHandleContext& ctx)
        : Logger(ctx.Ctx.Logger())
        , FastData(ctx.Ctx.GlobalContext().FastData().GetFastData<THowToSpellFastData>())
        , RequestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM))
        , Request(RequestProto, ctx.ServiceCtx)
        , NlgData(Logger, Request)
        , NlgWrapper(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request, ctx.Rng, ctx.UserLang))
        , Builder(&NlgWrapper)
        , BodyBuilder(Builder.CreateResponseBodyBuilder())
    {
    }

    std::unique_ptr<TScenarioRunResponse> MakeResponse() && {
        Run();
        return std::move(Builder).BuildResponse();
    }

private:
    void Run() {
        BodyBuilder.CreateAnalyticsInfoBuilder().SetProductScenarioName("how_to_spell");
        if (!Request.ClientInfo().IsSmartSpeaker()) {
            LOG_INFO(Logger) << "Surface is not smart_speaker";
            MakeIrrelevantResponse();
        } else if (const auto howToSpellFramePtr = Request.Input().FindSemanticFrame(HowToSpellFrameName)) {
            ProcessHowToSpellFrame(TFrame::FromProto(*howToSpellFramePtr));
        } else if (const auto verificationWordFramePtr = Request.Input().FindSemanticFrame(VerificationWordFrameName)) {
            ProcessVerificationWordFrame(TFrame::FromProto(*verificationWordFramePtr));
        } else {
            LOG_WARN(Logger) << "Valid semantic frame not found";
            MakeIrrelevantResponse();
        }
    }

    void ProcessHowToSpellFrame(const TFrame& frame) {
        if (const auto phraseSlotPtr = frame.FindSlot(PhraseSlotName)) {
            ProcessAnySlot(*phraseSlotPtr, FastData->PopularPhrasesToReplies,
                           /* requireDictionaryReply */ false,
                           /* needSpellingByLetters */ true);
        } else if (const auto letterSlotPtr = frame.FindSlot(LetterSlotName)) {
            ProcessAnySlot(*letterSlotPtr, FastData->LettersToReplies,
                           /* requireDictionaryReply */ true,
                           /* needSpellingByLetters */ false);
        } else if (const auto ruleSlotPtr = frame.FindSlot(RuleSlotName)) {
            ProcessAnySlot(*ruleSlotPtr, FastData->RulesToReplies,
                           /* requireDictionaryReply */ true,
                           /* needSpellingByLetters */ false);
        } else {
            LOG_WARN(Logger) << "No valid slot was found for search_how_to_spell frame";
            MakeIrrelevantResponse();
        }
    }

    void ProcessVerificationWordFrame(const TFrame& frame) {
        if (!FastData->EnableVerificationWordsQueries) {
            LOG_WARN(Logger) << "Requests for verification words are currently disabled";
            MakeIrrelevantResponse();
        } else if (const auto phraseSlotPtr = frame.FindSlot(PhraseSlotName)) {
            ProcessAnySlot(*phraseSlotPtr, FastData->PopularPhrasesToReplies,
                           /* requireDictionaryReply */ false,
                           /* needSpellingByLetters */ true);
        } else {
            LOG_WARN(Logger) << "No valid slot was found for search_what_is_verification_word frame";
            MakeIrrelevantResponse();
        }
    }

    void ProcessAnySlot(const TSlot& slot, const THashMap<TUtf16String, TPhraseReply>& dictionary,
                        bool requireDictionaryReply, bool needSpellingByLetters)
    {
        CollectPhrase(slot);

        if (PhraseIsInBanList()) {
            LOG_WARN(Logger) << "Phrase " << Phrase << " is blacklisted.";
            MakeIrrelevantResponse();
        } else if (!TryFillInDictionaryReply(dictionary) && requireDictionaryReply) {
            LOG_WARN(Logger) << "Couldn't find the required dictionary response for " << slot.Name << " slot";
            MakeIrrelevantResponse();
        } else if (needSpellingByLetters && !TryFillInPhraseWithPhonemes()) {
            LOG_WARN(Logger) << "Couldn't spell the phrase \"" << Phrase << "\"";
            MakeIrrelevantResponse();
        } else {
            RenderReplyPhrase();
        }
    }

    void CollectPhrase(const TSlot& slot) {
        auto rewrittenSlotValue = slot.Value.AsString();
        if (auto* entry = FastData->AsrRecognitionRewriteData.FindPtr(rewrittenSlotValue)) {
            rewrittenSlotValue = *entry;
        }

        TVector<TString> words;
        for (auto word : StringSplitter(rewrittenSlotValue).Split(' ').ToList<TString>()) {
            if (auto* entry = FastData->AsrRecognitionRewriteData.FindPtr(word)) {
                word = *entry;
            }
            words.push_back(word);
        }

        Phrase = UTF8ToWide(JoinStrings(words, " "));
    }

    bool PhraseIsInBanList() const {
        return Phrase.EndsWith(u"тся") || Phrase.EndsWith(u"ться");
    }

    bool TryFillInDictionaryReply(const THashMap<TUtf16String, TPhraseReply>& dictionary) {
        if (const auto* dictionaryReplyPtr = dictionary.FindPtr(Phrase)) {
            ReplyFromDictionaryText = dictionaryReplyPtr->TextReply;
            ReplyFromDictionaryVoice = dictionaryReplyPtr->VoiceReply;
            return true;
        }
        return false;
    }

    bool TryFillInPhraseWithPhonemes() {
        TVector<TString> phonemes;
        phonemes.reserve(Phrase.size());
        for (auto ch : Phrase) {
            if (const auto* phonemePtr = FastData->SymbolsToPhonemes.FindPtr(ch)) {
                phonemes.push_back(*phonemePtr);
            } else {
                return false;
            }
        }

        PhrasePhonemes = JoinSeq(FastData->LettersVoiceSeparator, phonemes);
        return true;
    }

    void RenderReplyPhrase() {
        NlgData.Context["phrase_phonemes"] = PhrasePhonemes;
        NlgData.Context["phrase"] = WideToUTF8(Phrase);
        NlgData.Context["reply_from_dictionary_text"] = ReplyFromDictionaryText;
        NlgData.Context["reply_from_dictionary_voice"] = ReplyFromDictionaryVoice;
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(NlgTemplateName, NlgRenderReplyPhraseName, {}, NlgData);
    }

    void MakeIrrelevantResponse() {
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(NlgTemplateName, NlgRenderReplyErrorName, {}, NlgData);
        Builder.SetIrrelevant();
    }

private:
    TRTLogger& Logger;
    const std::shared_ptr<const THowToSpellFastData> FastData;
    const TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;
    TNlgData NlgData;
    TNlgWrapper NlgWrapper;
    TRunResponseBuilder Builder;
    TResponseBodyBuilder& BodyBuilder;
    TUtf16String Phrase;
    TString ReplyFromDictionaryText;
    TString ReplyFromDictionaryVoice;
    TString PhrasePhonemes;

    static constexpr TStringBuf HowToSpellFrameName = "alice.search_how_to_spell";
    static constexpr TStringBuf VerificationWordFrameName = "alice.search_what_is_verification_word";
    static constexpr TStringBuf PhraseSlotName = "phrase";
    static constexpr TStringBuf LetterSlotName = "letter";
    static constexpr TStringBuf RuleSlotName = "rule";
    static constexpr TStringBuf NlgTemplateName = "how_to_spell";
    static constexpr TStringBuf NlgRenderReplyPhraseName = "render_reply";
    static constexpr TStringBuf NlgRenderReplyErrorName = "render_error";
};

} // namespace

void THowToSpellRunHandle::Do(TScenarioHandleContext& ctx) const {
    ctx.ServiceCtx.AddProtobufItem(*THowToSpellScenarioEngine{ctx}.MakeResponse(), RESPONSE_ITEM);
}

REGISTER_SCENARIO("how_to_spell",
                  AddHandle<THowToSpellRunHandle>()
                  .AddFastData<THowToSpellFastDataProto, THowToSpellFastData>("how_to_spell/how_to_spell.pb")
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NHowToSpell::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
