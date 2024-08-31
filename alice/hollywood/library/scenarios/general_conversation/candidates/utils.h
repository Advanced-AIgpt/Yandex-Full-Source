#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/library/experiments/experiments.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/generic/fwd.h>
#include <util/generic/maybe.h>
#include <util/string/cast.h>

namespace NMemento = ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood::NGeneralConversation {

TString PostProcessCandidateText(TStringBuf text, bool lowercase = false, const char* splitSet = " \t\r\n\f", ELanguage lang = ELanguage::LANG_RUS);
TVector<TString> ConstructContext(const TVector<TString>& dialogHistory, size_t contextLength, const TString& utterance, const TMaybe<TString>& reply = Nothing());
TString ConstructContextString(const TVector<TString>& dialogHistory, size_t contextLength, const TString& utterance, const TMaybe<TString>& reply = Nothing());

template <typename TRequestWrapper>
TVector<TString> GetDialogHistory(const TRequestWrapper& requestWrapper);

template <typename TRequestWrapper>
TString GetUtterance(const TRequestWrapper& requestWrapper, bool punctuation = false);

template <typename TRequestWrapper>
bool IsMovieDisscussionAllowedByDefault(const TRequestWrapper& requestWrapper, bool modalModeEnabled);

template <typename TRequestWrapper>
bool CountForEasterEggSuggest(const TRequestWrapper& requestWrapper);

bool RequiresSearchSuggests(const TScenarioRunRequestWrapper& requestWrapper, const TReplyInfo* replyInfo, const TClassificationResult& classificationResult);

template <typename TRequestWrapper>
TMaybe<NDatetime::TCivilSecond> GetClientTime(const TRequestWrapper& requestWrapper) {
    try {
        const auto& dateTime = TInstant::FromValue(requestWrapper.ClientInfo().Epoch * 1000 * 1000);
        const auto& tz = NDatetime::GetTimeZone(requestWrapper.ClientInfo().Timezone);
        return NDatetime::Convert(dateTime, tz);
    } catch (NDatetime::TInvalidTimezone e) {
        return Nothing();
    }
}

template <typename TRequestWrapper>
ui64 GetClientTimeSeconds(const TRequestWrapper& requestWrapper) {
    return requestWrapper.ClientInfo().Epoch;
}

template <typename TRequestWrapper>
ui64 GetServerTimeMs(const TRequestWrapper& requestWrapper) {
    if (requestWrapper.HasExpFlag(EXP_HW_GC_DEBUG_SERVER_TIME)) {
        return TInstant::Now().MilliSeconds();
    }

    return requestWrapper.BaseRequestProto().GetServerTimeMs();
}

TString GetCurrentDate(const TGeneralConversationRunContextWrapper& contextWrapper);


template <typename TRequestWrapper>
TMaybe<TString> GetConditionalExperimentValue(const TRequestWrapper& requestWrapper, const TSessionState& sessionState, TStringBuf prefix) {
    RE2 flagCapturingExpression{TString{prefix} + R"((\[(.*)\])?=(.*))"};
    for (const auto& [flag, _] : requestWrapper.ExpFlags()) {
        TString mode;
        TString value;
        if (RE2::FullMatch(flag, flagCapturingExpression, nullptr, &mode, &value)) {
            if (!mode || mode == "all") {
                return value;
            } else if (mode == "pure" && sessionState.GetModalModeEnabled()) {
                return value;
            } else if (mode == "general" && !sessionState.GetModalModeEnabled()) {
                return value;
            }
        }
    }
    return Nothing();
}

template <typename T, typename TRequestWrapper>
TMaybe<T> GetExperimentTypedValue(const TRequestWrapper& requestWrapper, const TSessionState& sessionState, TStringBuf prefix) {
    const auto& value = GetConditionalExperimentValue(requestWrapper, sessionState, prefix);
    if (!value) {
        return Nothing();
    }
    return FromString<T>(value.GetRef());
}

template <typename T>
TMaybe<T> GetExperimentTypedValue(const THashMap<TString, TMaybe<TString>>& flags, TStringBuf prefix) {
    const auto& value = NAlice::GetExperimentValueWithPrefix(flags, prefix);
    if (!value) {
        return Nothing();
    }
    return FromString<T>(value.GetRef());
}

template <typename TCandidate>
void DeduplicateCandidates(TVector<TCandidate>* candidates) {
    THashSet<TString> foundKeys;
    foundKeys.reserve(candidates->size());
    const auto eraseFunc = [&foundKeys](const TCandidate& candidate) {
        auto key = PostProcessCandidateText(candidate.GetText(), true, "!\"#$%&'()*+, -./:;<=>?@[\\]^_`{|}~");
        const auto found = foundKeys.contains(key);
        if (!found)
            foundKeys.insert(std::move(key));
        return found;
    };

    EraseIf(*candidates, eraseFunc);
}

TString DeletePrepositions(TStringBuf text, const TVector<TString>& prepositions);
TString Capitalize(TStringBuf text);
TUtf16String InflectToAcc(const TUtf16String& words);
TString MakeQuestionVoice(const TString& question);

template <typename TRequestWrapper>
size_t GetDialogHistorySize(const TRequestWrapper& requestWrapper, const TSessionState& sessionState);

const NMemento::TUserConfigs GetMementoUserConfig(const TScenarioBaseRequestWrapper& baseRequestWrapper);

} // namespace NAlice::NHollywood::NGeneralConversation
