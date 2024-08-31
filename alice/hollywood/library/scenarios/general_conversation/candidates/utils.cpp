#include "utils.h"

#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/memento/proto/api.pb.h>

#include <kernel/inflectorlib/phrase/simple/simple.h>

#include <dict/dictutil/dictutil.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/subst.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

TString FilterTagsInResponse(TString text) {
    static const RE2 re{"<.*>"};
    RE2::GlobalReplace(&text, re, "");
    return text;
}
} // namepace

TString PostProcessCandidateText(TStringBuf text, bool lowercase, const char* splitSet, ELanguage lang) {
    const auto& words = StringSplitter(text).SplitBySet(splitSet).SkipEmpty().ToList<TString>();
    auto endWord = words.end();
    if (words.size() > 0 && words.back() == "_EOS_") {
        endWord -= 1;
    }

    TString result = JoinRange(" ", words.begin(), endWord);
    SubstGlobal(result, "\\\"", "\"");
    SubstGlobal(result, "\\'", "'");

    if (lowercase) {
        result = WideToUTF8(ToLower(lang, UTF8ToWide(result)));
    }

    return result;
}

TString GetCurrentDate(const TGeneralConversationRunContextWrapper& contextWrapper) {
    time_t now = GetServerTimeMs(contextWrapper.RequestWrapper()) / 1000;
    char buffer[80];
    struct tm*timeinfo;
    timeinfo = localtime(&now);

    strftime(buffer,sizeof(buffer),"%Y-%m-%d", timeinfo);
    TString str(buffer);
    return str;
}

TVector<TString> ConstructContext(const TVector<TString>& dialogHistory, size_t contextLength, const TString& utterance, const TMaybe<TString>& reply) {
    TVector<TString> context;
    context.reserve(contextLength);
    const auto currentDialogLength = reply ? 2 : 1;
    const auto contextStartIndex =
        dialogHistory.size() + currentDialogLength > contextLength
        ? Min(dialogHistory.size() + currentDialogLength - contextLength, dialogHistory.size())
        : 0;
    ForEach(dialogHistory.begin() + contextStartIndex, dialogHistory.end(), [&context] (const auto& message) { context.push_back(message); });
    context.push_back(utterance);
    if (reply) {
        context.push_back(reply.GetRef());
    }
    for (auto& phrase : context) {
        SubstGlobal(phrase, "\n", " ");
    }

    return context;
}

TString ConstructContextString(const TVector<TString>& dialogHistory, size_t contextLength, const TString& utterance, const TMaybe<TString>& reply) {
    return JoinSeq("\n", ConstructContext(dialogHistory, contextLength, utterance, reply));
}

template <typename TRequestWrapper>
TVector<TString> GetDialogHistory(const TRequestWrapper& requestWrapper) {
    const auto* dialogHistoryDataSource = requestWrapper.GetDataSource(EDataSourceType::DIALOG_HISTORY);
    Y_ENSURE(dialogHistoryDataSource, "DialogHistoryDataSource not found in the run request");

    TVector<TString> result;
    const auto& dialogTurns = dialogHistoryDataSource->GetDialogHistory().GetDialogTurns();
    result.reserve(2*dialogTurns.size());
    for (const auto& dialogTurn : dialogTurns) {
        result.push_back(dialogTurn.GetRequest());
        result.push_back(FilterTagsInResponse(dialogTurn.GetResponse()));
    }

    return result;
}

template TVector<TString> GetDialogHistory(const TScenarioRunRequestWrapper&);

template <typename TRequestWrapper>
TString GetUtterance(const TRequestWrapper& requestWrapper, bool punctuation) {
    if (const auto callbackFrame = GetCallbackFrame(requestWrapper.Input().GetCallback())) {
        if (callbackFrame->Name() == FRAME_GC_SUGGEST) {
            if (const auto requestSlot = callbackFrame->FindSlot(SLOT_SUGGEST_TEXT)) {
                return requestSlot->Value.AsString();
            }
        }
    }

    if (requestWrapper.HasExpFlag(EXP_HW_ENABLE_GC_RAW_VOICE_UTTERANCE) || punctuation) {
        const auto& eventCase = requestWrapper.Input().Proto().GetEventCase();
        const auto& voiceEvent = requestWrapper.Input().Proto().GetVoice();
        if (eventCase == NScenarios::TInput::kVoice && voiceEvent.AsrDataSize() > 0) {
            return voiceEvent.GetAsrData(0).GetUtterance();
        }
    }

    if (requestWrapper.HasExpFlag(EXP_HW_ENABLE_GC_RAW_TEXT_UTTERANCE)) {
        const auto& eventCase = requestWrapper.Input().Proto().GetEventCase();
        const auto& textEvent = requestWrapper.Input().Proto().GetText();
        if (eventCase == NScenarios::TInput::kText) {
            return textEvent.GetRawUtterance();
        }

    }

    return requestWrapper.Input().Utterance();
}

template TString GetUtterance(const TScenarioRunRequestWrapper&, bool);
template TString GetUtterance(const TScenarioApplyRequestWrapper&, bool);


template <typename TRequestWrapper>
bool IsMovieDisscussionAllowedByDefault(const TRequestWrapper& requestWrapper, bool modalModeEnabled) {
    if (!modalModeEnabled) {
        return false;
    }

    return !requestWrapper.HasExpFlag(EXP_HW_DISABLE_GC_MOVIE_DISCUSSIONS);
}

template bool IsMovieDisscussionAllowedByDefault(const TScenarioRunRequestWrapper&, bool);
template bool IsMovieDisscussionAllowedByDefault(const TScenarioApplyRequestWrapper&, bool);


template <typename TRequestWrapper>
bool CountForEasterEggSuggest(const TRequestWrapper& requestWrapper) {
    if (requestWrapper.HasExpFlag(EXP_HW_GC_ENABLE_EASTER_EGG_AMANDA)) {
        return requestWrapper.Input().IsTextInput();
    } else {
        return requestWrapper.Input().Proto().GetText().GetFromSuggest();
    }
}

template bool CountForEasterEggSuggest(const TScenarioRunRequestWrapper&);
template bool CountForEasterEggSuggest(const TScenarioApplyRequestWrapper&);


bool RequiresSearchSuggests(const TScenarioRunRequestWrapper& requestWrapper, const TReplyInfo* replyInfo, const TClassificationResult& classificationResult) {
    const bool hasItsOwnSuggests = requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SUGGESTS) && replyInfo && replyInfo->GetIntent() == FRAME_MOVIE_DISCUSS;
    return classificationResult.GetHasSearchSuggestsRequest() && !hasItsOwnSuggests;
}


TString DeletePrepositions(TStringBuf text, const TVector<TString>& prepositions) {
    auto newText = text;
    for (auto prep: prepositions) {
        prep = prep + " ";
        if (text.find(prep) == 0) {
            newText = text.substr(prep.size());
            break;
        }
    }
    newText = StripString(newText);
    return ToString(newText);
}


TString Capitalize(TStringBuf text) {
    TStringBuilder newTextBuilder;
    for (const auto& word: StringSplitter(text).SplitByString(" ")) {
        TUtf16String wideWord = UTF8ToWide(word);
        if (!IsIn(PREPOSITIONS_AND_CONJUNCTIONS, wideWord)) {
            ToTitle(wideWord);
        }
        newTextBuilder << WideToUTF8(wideWord) << " ";
    }
    TString newText = StripString(ToString(newTextBuilder));
    return newText;
}

TUtf16String InflectToAcc(const TUtf16String& words) {
    if (const auto* result = ACCUSATIVE_FIXLIST.FindPtr(words)) {
        return *result;
    }
    return NInfl::TSimpleInflector().Inflect(words, "acc");
}


TString MakeQuestionVoice(const TString& question) {
    TVector<TString> words;
    int wordIndex = 0;
    for (const auto& word : StringSplitter(question).SplitByString(" ")) {
        auto wordLowered = UTF8ToWide(word);
        Y_UNUSED(ToLower(wordLowered));

        if (wordIndex++ < 2 && IsIn(QUESTIONS_WORDS, wordLowered)) {
            words.push_back(TString(word) + "<[accented]>");
        } else {
            words.push_back(TString(word));
        }
    }

    return JoinSeq(" ", words);
}


const NMemento::TUserConfigs GetMementoUserConfig(const TScenarioBaseRequestWrapper& baseRequestWrapper) {
    return baseRequestWrapper.BaseRequestProto().GetMemento().GetUserConfigs();
}

template <typename TRequestWrapper>
size_t GetDialogHistorySize(const TRequestWrapper& requestWrapper, const TSessionState& sessionState) {
    return GetExperimentTypedValue<size_t>(requestWrapper, sessionState, EXP_HW_SET_CONTEXT_LENGTH).GetOrElse(CONTEXT_LENGTH);
}

template size_t GetDialogHistorySize(const TScenarioRunRequestWrapper&, const TSessionState&);
template size_t GetDialogHistorySize(const TScenarioApplyRequestWrapper&, const TSessionState&);

} // namespace NAlice::NHollywood::NGeneralConversation
