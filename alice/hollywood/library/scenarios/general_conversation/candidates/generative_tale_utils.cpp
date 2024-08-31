#include "generative_tale_utils.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/library/experiments/experiments.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/user_configs.pb.h>

#include <util/string/strip.h>

#include <regex>


namespace  {
bool ParseCandidate(const TString& text, std::smatch& groups) {
    const std::regex pattern(R"(^([^\(]+)\(([^:]+):\s*(.+?) или ([^?\)]+)[?\)]+)");
    std::smatch m;
    if (std::regex_search(text.data(), m, pattern)) {
        groups = std::move(m);
        return true;
    }
    return false;
}

} // namespace


namespace NAlice::NHollywood::NGeneralConversation {

void ParseGenerativeTaleQuestion(const TVector<TSeq2SeqReplyCandidate>& seq2seqResponse, TReplyInfo& replyInfo) {
    std::smatch groups;
    TString tale;
    // Смотрим все ответы seq2seq. Среди них могут быть ответы с вопросами, но иногда их нет.
    for (auto response: seq2seqResponse) {
        TString text = response.GetText();
        // Проверяем, если в ответе вопрос. Если есть, возвращаем его.
        if (ParseCandidate(text, groups)) {                                                                 // Текст. (Вопрос: А или Б? ...) ...
            tale = groups[1].str();                                                                         // Текст.
            tale = TaleDropEmptyStart(tale);
            if (tale.size() < TALE_MIN_SIZE) {
                continue;
            }

            TString question = groups[2].str() + ": " + groups[3].str() + " или " + groups[4].str() + "?";  // Вопрос: А или Б? ...
            replyInfo.MutableGenerativeTaleReply()->SetText(tale);
            replyInfo.MutableGenerativeTaleReply()->MutableTaleState()->SetActiveQuestion(question);
            replyInfo.MutableGenerativeTaleReply()->MutableTaleState()->ClearActiveAnswers();
            replyInfo.MutableGenerativeTaleReply()->MutableTaleState()->AddActiveAnswers(groups[3].str());  // А
            replyInfo.MutableGenerativeTaleReply()->MutableTaleState()->AddActiveAnswers(groups[4].str());  // Б
            return;
        } else { 
            // Если в ответе нет вопроса, то добавляем его в возможное продолжение.
            // Если во всех ответах не будет вопроса, то возвращаем сказку, которая подходит под критерий размера
            text = TaleDropEmptyStart(text);
            if (text.size() > TALE_MIN_SIZE) {
                tale = text;
            }
        }
    }
    // Если не удалось найти продолжение с вопросом и все продолжения меньше TALE_MIN_SIZE
    // То выбираем первую гипотезу.
    if (tale.empty()) {
        tale = seq2seqResponse[0].GetText();
        tale = TaleDropEmptyStart(tale);
    }
    TString question = TALE_QUESTION_POSTFIX;
    // Проверяем, сгенерировали ли мы хотя бы открытый вопрос
    const size_t openBracketPos = tale.find_last_of("(");
    if (openBracketPos != TString::npos) {
        const size_t colonPos = tale.find_last_of(":");
        if ((colonPos != TString::npos) && (colonPos > openBracketPos)) {
            TString question = tale.substr(openBracketPos + 1, colonPos) + "?";
        }
        // Удаляем из текста открытый вопрос
        tale = tale.resize(openBracketPos);
    } else {
        // Находим последнее предложение в тексте
        const size_t rPos = tale.find_last_of(EOS_CHAR);
        if (rPos != TString::npos) {
            tale = tale.resize(rPos);
        }
    }

    replyInfo.MutableGenerativeTaleReply()->SetText(tale);
    replyInfo.MutableGenerativeTaleReply()->MutableTaleState()->SetActiveQuestion(question);
    replyInfo.MutableGenerativeTaleReply()->MutableTaleState()->ClearActiveAnswers();
}

TString GetStageName(const TGenerativeTaleState::EStage& stage) {
    const ::google::protobuf::EnumDescriptor *descriptor = TGenerativeTaleState_EStage_descriptor();
    TString name = descriptor->FindValueByNumber(stage)->name();

    return name;
}

TGenerativeTaleState::EStage MoveToPreviousQuestion(const TGenerativeTaleState::EStage& stage) {
    switch (stage) {
        case TGenerativeTaleState::Undefined:
            return stage;
        case TGenerativeTaleState::UndefinedCharacter:
            return stage;
        case TGenerativeTaleState::FirstQuestion:
            return stage;
        case TGenerativeTaleState::ClosedQuestion:
            return TGenerativeTaleState::FirstQuestion;
        case TGenerativeTaleState::OpenQuestion:
            return TGenerativeTaleState::ClosedQuestion;
        case TGenerativeTaleState::Sharing:
            return TGenerativeTaleState::OpenQuestion;
        case TGenerativeTaleState::Stop:
            return stage;
        default:
            return stage;
    }
}

TGenerativeTaleState::EStage MoveToNextQuestion(const TGenerativeTaleState::EStage& stage) {
    switch (stage) {
        case TGenerativeTaleState::Undefined:
            return TGenerativeTaleState::FirstQuestion;
        case TGenerativeTaleState::UndefinedCharacter:
            return TGenerativeTaleState::FirstQuestion;
        case TGenerativeTaleState::FirstQuestion:
            return TGenerativeTaleState::ClosedQuestion;
        case TGenerativeTaleState::ClosedQuestion:
            return TGenerativeTaleState::OpenQuestion;
        case TGenerativeTaleState::OpenQuestion:
            return TGenerativeTaleState::Sharing;
        case TGenerativeTaleState::Sharing:
            return TGenerativeTaleState::UndefinedQuestion;
        case TGenerativeTaleState::UndefinedQuestion:
            return TGenerativeTaleState::ClosedQuestion;
        case TGenerativeTaleState::Stop:
            return stage;
        default:
            return stage;
    }
}

TString TaleAddQuestion(const TString& question, const TString& answer, const TString& prefix) {
    size_t colonPos = question.find(":");
    TString questionWoOptions = question;
    if (colonPos != TString::npos) {
        questionWoOptions = questionWoOptions.resize(colonPos) + "?";
    }
    return Strip(prefix) + " (" + questionWoOptions + " " + answer + ")";
}

TString TaleDropEmptyStart(TString text) {
    for (size_t idx = 0; idx < text.size(); ++idx) {
        if (! (IsAsciiSpace(text[idx]) || EOS_CHAR.find(text[idx]) < EOS_CHAR.size())) {
            if (idx > 0) {
                text = text.substr(idx - 1);
            }
            break;
        }
    }
    return text;
}

TString TaleDropEnd(TString text) {
    const size_t rPos = text.find_last_of(EOS_CHAR);
    if (rPos != TString::npos)
        text = text.resize(rPos) + ".";

    return text;
}

template <typename TContextWrapper>
bool NeedTalesOnboarding(const TContextWrapper& contextWrapper, const TGenerativeTaleState& taleState) {
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    const auto& testUsage = NAlice::GetExperimentValueWithPrefix(requestWrapper.ExpFlags(), EXP_HW_GENERATIVE_TALE_USAGE_COUNTER);
    const auto usageCounter = testUsage ? FromString<ui64>(testUsage.GetRef()) : GetMementoUserConfig(requestWrapper).GetGenerativeTale().GetUsageCounter();

    return !taleState.GetHadOnboarding() && !taleState.GetHasObscene() && usageCounter < GENERATIVE_TALES_ONBOARDING_COUNT;
}

template bool NeedTalesOnboarding(const TGeneralConversationRunContextWrapper&, const TGenerativeTaleState&);
template bool NeedTalesOnboarding(const TGeneralConversationApplyContextWrapper&, const TGenerativeTaleState&);

TString MakeSharedLinkImageUrl(const TString& avatarsId, bool full) {
    const TString postfix = full ? "/catalogue-banner-x2" : "/catalogue-banner-x1";

    return TString::Join("http://avatars.mds.yandex.net", avatarsId, postfix);
}

} // namespace NAlice::NHollywood::NGeneralConversation
