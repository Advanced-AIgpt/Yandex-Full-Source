#include "asr_hypothesis_picker.h"
#include "scenarios_detectors.h"
#include "util.h"

#include <util/string/builder.h>
#include <util/generic/hash_set.h>

namespace NAlice {

namespace {

void LogParsedHypotheses(TLogAdapter* logger, const TVector<TAsrHypothesis>& asrHypotheses) {
    auto hypothesesUtterancesLog = TStringBuilder() << "PickBestHypothesis: Got following utterances: ";
    for (const auto& hypo : asrHypotheses) {
        hypothesesUtterancesLog << "\"" << hypo.Utterance() << "\",    ";
    }
    if (logger) LOG_ADAPTER_INFO(*logger) << hypothesesUtterancesLog;
}

class TAsrHypothesesHeuristic {
public:
    struct TResult {
        bool IsApplicable = false;
        size_t BestHypothesisIndex = NPOS;

        static TResult NotApplicable() {
            return {};
        }
    };

    virtual TResult Run(const TVector<TAsrHypothesis>& asrHypotheses, TLogAdapter* logger) = 0;
    virtual ~TAsrHypothesesHeuristic() = default;
};

}  // namespace

namespace NShortVsLong {


class TWordsCounter {
public:
    explicit TWordsCounter(const TVector<TUtf16String>& words = {}) {
        for (const auto& word : words) {
            Counter[word] += 1;
        }
    }

    TWordsCounter operator-(const TWordsCounter& rhs) const {
        TWordsCounter result;
        for (const auto& [word, count] : Counter) {
            int thisWordResultCount = count;
            if (rhs.Counter.contains(word)) {
                thisWordResultCount -= rhs.Counter.at(word);
            }
            if (thisWordResultCount > 0) {
                result.Counter[word] = thisWordResultCount;
            }
        }
        return result;
    }

    [[nodiscard]] const THashMap<TUtf16String, int>& Items() const {
        return Counter;
    }

private:
    THashMap<TUtf16String , int> Counter;
};


class THeuristic : public TAsrHypothesesHeuristic {
public:
    explicit THeuristic(TMaybe<TUsefulContextForReranker::TIoTInfo> ioTInfo = {})
        : IoTInfo(std::move(ioTInfo))
    {
    }

    TResult Run(const TVector<TAsrHypothesis>& asrHypotheses, TLogAdapter* logger) override {
        AsrHypotheses = &asrHypotheses;
        BestHypothesisId = 0;

        if (ZeroHypothesisIsBlacklisted()) {
            if (logger) LOG_ADAPTER_INFO(*logger) <<
                "ShortVsLong Heuristic is not applicable: 0th hypothesis is blacklisted\n";
            return TResult::NotApplicable();
        }

        if (!AdvanceIsNeeded()) {
            if (logger) LOG_ADAPTER_INFO(*logger) << "ShortVsLong Heuristic is not applicable: Advance is not needed\n";
            return TResult::NotApplicable();
        }

        FindBestHypothesisId();

        if (BestHypothesisAdvanceIsTooMassive()) {
            if (logger) LOG_ADAPTER_INFO(*logger) << "ShortVsLong Heuristic: "
                "Got bestHypothesisId = " << BestHypothesisId << ", but resetting it to 0\n";
            return TResult::NotApplicable();
        }

        if (BestHypothesisId) {
            if (logger) LOG_ADAPTER_INFO(*logger)
                << "ShortVsLong Heuristic is OK; won hypothesis " << BestHypothesisId << "\n";
            return {true, BestHypothesisId};
        } else {
            if (logger) LOG_ADAPTER_INFO(*logger) << "ShortVsLong Heuristic is not applicable.\n";
            return TResult::NotApplicable();
        }
    }

private:
    void FindBestHypothesisId() {
        BestHypothesisId = 0;
        for (size_t i = 1; i < 5 && i < AsrHypotheses->size(); ++i) {
            if (IsSubHypo((*AsrHypotheses)[BestHypothesisId], (*AsrHypotheses)[i])) {
                BestHypothesisId = i;
            }
        }
    }

    bool BestHypothesisAdvanceIsTooMassive() const {
        if (BestHypothesisId == 0) {
            return false;
        }

        const auto& testingHypothesis = (*AsrHypotheses)[BestHypothesisId];
        const auto subHypoInfo = ComputeSubHypoInfo((*AsrHypotheses)[0], testingHypothesis);

        // Rubbish words get weight 1, non-rubbish -- 0.7
        const float nAddedWordsWeightedAccordingToRubbishness =
                static_cast<float>(subHypoInfo.AddedWords.ysize()) * 0.7f +
                static_cast<float>(CountRubbishWords(subHypoInfo.AddedWords)) * 0.3f;
        const bool addedTooManyWords = nAddedWordsWeightedAccordingToRubbishness > 6;
        return addedTooManyWords;
    }

    [[nodiscard]] bool ZeroHypothesisIsBlacklisted() const {
        auto&& zeroHypo = (*AsrHypotheses)[0];
        const auto userIoTScenariosAndTriggers =
                IoTInfo.Defined() ? IoTInfo.Get()->IoTScenariosNamesAndTriggers : TVector<TString>{};
        return (
            IsIoTCommand(zeroHypo, userIoTScenariosAndTriggers) ||
            IsItemSelectorCommand(zeroHypo) ||
            IsArithmeticsCommand(zeroHypo) ||
            IsShortCommand(zeroHypo)
        );
    }

    [[nodiscard]] bool AdvanceIsNeeded() const {
        const auto& zeroHypo = (*AsrHypotheses)[0];
        const auto zeroHypoWords = TWordsCounter(zeroHypo.Words);
        const auto zeroHypoText = zeroHypo.Utterance();

        if (zeroHypoText.size() >= 50 || zeroHypo.Words.ysize() >= 9 || zeroHypo.Words.ysize() == 0) {
            return false;
        }

        const int analyzeFirstK = 10;
        float hyposLongerThanFirstTotalWeight = 0;
        float allHyposTotalWeight = 0;
        const TVector<float> hardcodedWeights = {0, 3.f/4, 1.f/2, 1.f/3, 1.f/3, 1.f/4, 1.f/5, 1.f/5, 1.f/7, 1.f/7, 1.f/8};
        for (int i = 1; i < Min(analyzeFirstK + 1, AsrHypotheses->ysize()); ++i) {
            auto weight = hardcodedWeights[i];
            auto&& thisHypo = (*AsrHypotheses)[i];

            const auto thisHypoWords = TWordsCounter(thisHypo.Words);
            const auto thisHypoWordsMinusZeroHypoWords = thisHypoWords - zeroHypoWords;
            const auto zeroHypoWordsMinusThisHypoWords = zeroHypoWords - thisHypoWords;
            float score = 0;
            for (const auto& [word, count] : thisHypoWordsMinusZeroHypoWords.Items()) {
                const auto wordWithEndingCut = word.substr(0, word.size() - 1);
                if (!zeroHypoText.Contains(wordWithEndingCut) &&
                    !IsRubbishWord(word) &&
                    !IsIn(CanNotBeAloneWords, word))
                {
                    score += static_cast<float>(count);
                }
            }
            for (const auto& [word, count] : zeroHypoWordsMinusThisHypoWords.Items()) {
                if (!IsRubbishWord(word)) {
                    score -= static_cast<float>(count);
                }
            }
            if (score > 0) {
                hyposLongerThanFirstTotalWeight += weight;
            }

            allHyposTotalWeight += weight;
        }

        const bool zeroHypoIsASingleWordNotAskingForAdvance = (
            zeroHypo.Words.ysize() == 1 &&
            !IsIn(WordsAskingForAdvance, zeroHypo.Words[0])
        );
        const float mulFactor = zeroHypoIsASingleWordNotAskingForAdvance ? 1.3 : 1.85;
        return allHyposTotalWeight <= hyposLongerThanFirstTotalWeight * mulFactor;
    }

    static int CountNonRubbishWords(const TVector<TUtf16String>& words) {
        return static_cast<int>(CountIf(words, [&](const auto& word) { return !IsRubbishWord(word); }));
    }

    static int CountRubbishWords(const TVector<TUtf16String>& words) {
        return static_cast<int>(CountIf(words, [&](const auto& word) { return IsRubbishWord(word); }));
    }

    struct TSubHypoInfo {
        TVector<TUtf16String> AddedWords{};
        int NAddedWordsThatAlreadyWerePresent = 0;
        int NWordsAddedToTheBeginning = 0;
        bool FullyMatched = false;
    };

    static TSubHypoInfo ComputeSubHypoInfo(const TAsrHypothesis& baseline, const TAsrHypothesis& testing) {
        const auto& bWords = baseline.Words;
        const auto& tWords = testing.Words;

        TSubHypoInfo result;
        auto bWordIter = bWords.begin();
        for (const auto& tWord : tWords) {
            if (bWordIter != bWords.end() && tWord == *bWordIter) {
                ++bWordIter;
            } else {
                result.AddedWords.push_back(tWord);
                result.NWordsAddedToTheBeginning += bWordIter == bWords.begin();

                if (IsIn(bWords, tWord)) {
                    result.NAddedWordsThatAlreadyWerePresent++;
                }
            }
        }
        result.FullyMatched = bWordIter == bWords.end();
        return result;
    }

    static bool IsSubHypo(const TAsrHypothesis& baseline, const TAsrHypothesis& testing) {
        const auto subHypoInfo = ComputeSubHypoInfo(baseline, testing);

        if (!subHypoInfo.FullyMatched || subHypoInfo.AddedWords.empty()) {
            return false;
        }

        const int nAdditionalWords = subHypoInfo.AddedWords.ysize();
        const int nNewAdditionalWords = nAdditionalWords - subHypoInfo.NAddedWordsThatAlreadyWerePresent;

        const bool firstBaselineWordIsAlice = baseline.Words.ysize() > 0 && IsIn({u"алиса", u"алис", u"яндекс"}, baseline.Words[0]);
        const bool addedBeforeFirstAlice = subHypoInfo.NWordsAddedToTheBeginning > 0 && firstBaselineWordIsAlice;
        if (addedBeforeFirstAlice) {
            return false;
        }

        const int nAddedRubbishWords = static_cast<int>(CountIf(subHypoInfo.AddedWords, IsRubbishWord));
        const bool tooManyRubbishWords = 1.5 * nAddedRubbishWords >= nNewAdditionalWords;
        if (tooManyRubbishWords) {
            return false;
        }

        const bool hasForbiddenWords = AnyOf(subHypoInfo.AddedWords, [&](const auto& word) {
            return IsIn(ForbiddenWords, word);
        });
        if (hasForbiddenWords) {
            return false;
        }

        const bool addedWordThatCannotBeAddedAlone = IsIn(CanNotBeAloneWords, subHypoInfo.AddedWords[0]);
        const bool addedSingleWordNotAllowedToBeAlone = nAdditionalWords == 1 && addedWordThatCannotBeAddedAlone;
        if (addedSingleWordNotAllowedToBeAlone) {
            return false;
        }

        const auto baselineUtterance = baseline.Utterance();
        if (AllOf(subHypoInfo.AddedWords, [&](const auto& word) {
            return baselineUtterance.Contains(word);
        })) {
            return false;
        }

        return true;
    }

    static bool IsRubbishWord(const TUtf16String& word) {
        return word.Size() == 1 || IsIn(RubbishWords, word);
    }

private:
    const TMaybe<TUsefulContextForReranker::TIoTInfo> IoTInfo;

    const TVector<TAsrHypothesis>* AsrHypotheses = nullptr;
    size_t BestHypothesisId = NPOS;

    static const THashSet<TUtf16String> ForbiddenWords;
    static const THashSet<TUtf16String> CanNotBeAloneWords;
    static const THashSet<TUtf16String> RubbishWords;
    static const THashSet<TUtf16String> WordsAskingForAdvance;
};

const THashSet<TUtf16String> THeuristic::ForbiddenWords {
    u"клип", u"клипа", u"клипов", u"клипы",
    u"видео", u"видос", u"видосик",
    u"фильм", u"мульт", u"мультик",
    u"фильмец", u"фильм", u"фильмов", u"фильму", u"фильмы",
    u"ролик", u"ролики", u"ролика", u"роликов",
    u"мульт", u"мультик", u"мультик", u"мультфильм",

    u"бля", u"блядь", u"блять",
    u"хуй", u"хуйня", u"нахуй", u"хуя", u"нахуя",
    u"сука", u"пизда", u"пиздец", u"мать", u"матерь", u"матери", u"ёб", u"еб", u"ебать", u"заебывала",
    u"дура", u"дурак", u"дурочка", u"дурачок", u"идиот", u"идиоты", u"идиотка", u"идиотки",
    u"глупый", u"глупая", u"глупые", u"тупой", u"тупая", u"тупые", u"глухая", u"глухой",
    u"хер", u"хера", u"херня", u"нахера", u"нихера", u"нахер",
    u"нафига", u"нифига", u"фига", u"нафиг", u"нефиг",

    u"<unk>",
};

const THashSet<TUtf16String> THeuristic::CanNotBeAloneWords {
    u"группа", u"группу", u"группы",
    u"звук", u"звука", u"звуки",
    u"музыка", u"музыке", u"музыки", u"музыку",
    u"песня", u"песни", u"песне", u"песню",
    u"трек", u"трэк", u"трека", u"трэка", u"треки", u"трэки", u"треков", u"трэков",
    u"игра", u"игру", u"игры",
};

const THashSet<TUtf16String> THeuristic::RubbishWords{
    u"алиса", u"алис",

    /* ForbiddenWords */
    u"клип", u"клипа", u"клипов", u"клипы",
    u"видео", u"видос", u"видосик",
    u"фильм", u"мульт", u"мультик",
    u"фильмец", u"фильм", u"фильмов", u"фильму", u"фильмы",
    u"ролик", u"ролики", u"ролика", u"роликов",
    u"мульт", u"мультик", u"мультик", u"мультфильм",

    u"бля", u"блядь", u"блять",
    u"хуй", u"хуйня", u"нахуй", u"хуя", u"нахуя",
    u"сука", u"пизда", u"пиздец", u"мать", u"матерь", u"матери", u"ёб", u"еб", u"ебать", u"заебывала",
    u"дура", u"дурак", u"дурочка", u"дурачок", u"идиот", u"идиоты", u"идиотка", u"идиотки",
    u"глупый", u"глупая", u"глупые", u"тупой", u"тупая", u"тупые", u"глухая", u"глухой",
    u"хер", u"хера", u"херня", u"нахера", u"нихера", u"нахер",
    u"нафига", u"нифига", u"фига", u"нафиг", u"нефиг",

    u"<unk>",

    /* Pronouns */
    u"я", u"мы", u"он", u"она", u"они", u"оно", u"ты", u"вы",
    u"мне", u"нам", u"ему", u"ей", u"ему", u"им", u"тебе", u"вам",
    u"нашей", u"нашему", u"нашим", u"его", u"её", u"ее", u"мою", u"мое",
    u"их", u"твоей", u"твоему", u"твоим", u"вашим", u"вашему", u"вашей",
    u"твою", u"тебя", u"себя", u"меня", u"их", u"нас", u"него", u"неё", u"этого", u"этой", u"этих",
    u"кто", u"что", u"вас",

    /* Conjunctions */
    u"всё", u"все", u"всю", u"вся", u"весь", u"как", u"так", u"вот",
    u"то", u"это", u"та", u"эта", u"этот", u"эти", u"тот",

    /* Prepositions */
    u"из", u"за", u"от", u"для", u"на", u"над", u"по", u"под", u"во", u"до", u"ко", u"из", u"за", u"без",

    u"какая", u"такая", u"какое", u"такое", u"какие", u"такие", u"какой", u"такой",
    u"быть", u"будет", u"был", u"есть",
    u"ау", u"оу", u"ок", u"окей", u"да", u"нет", u"не", u"неа", u"ага", u"угу", u"ого", u"ой",
    u"яр", u"ну", u"ам",
    u"тут", u"там", u"здесь",
    u"харе", u"харэ",
    u"стоп", u"звук", u"слушать", u"смотреть", u"читать", u"пока",
    u"еще", u"ещё",
    u"ооо", u"ааа", u"лол", u"фу", u"тьфу", u"надо", u"иди",
};


const THashSet<TUtf16String> THeuristic::WordsAskingForAdvance{
    u"включи", u"поставь", u"как", u"кто", u"что", u"сколько", u"почему", u"зачем",
};

}  // namespace NShortVsLong

///////////////////////////////////////////////////////////////////////////////


namespace {

size_t DoPickBestHypothesis(const TVector<TAsrHypothesis>& asrHypotheses,
                            const TUsefulContextForReranker& context,
                            TLogAdapter* logger)
{
    Y_ENSURE(!asrHypotheses.empty());
    LogParsedHypotheses(logger, asrHypotheses);

    if (context.ClientInfo.Defined() && (context.ClientInfo.Get()->IsNavigator() || context.ClientInfo.Get()->IsYaAuto()))
    {
        if (logger) LOG_ADAPTER_INFO(*logger) << "PickBestHypothesis: Reranking is not applied for navi/auto surfaces.";
        return 0;
    }

    NShortVsLong::THeuristic shortVsLongHeuristic{context.IoTInfo};
    if (auto result = shortVsLongHeuristic.Run(asrHypotheses, logger); result.IsApplicable) {
        return result.BestHypothesisIndex;
    }
    return 0;
}

}  // namespace


size_t PickBestHypothesis(TVector<TAsrHypothesisWideWords> asrHypothesesWords,
                          const TUsefulContextForReranker& context, TLogAdapter* logger)
{
    TVector<TAsrHypothesis> asrHypotheses;
    for (const auto& asrHypothesisWords : asrHypothesesWords) {
        asrHypotheses.emplace_back(asrHypothesisWords);
    }

    return DoPickBestHypothesis(asrHypotheses, context, logger);
}

size_t PickBestHypothesis(const TVector<TAsrHypothesisWords>& asrHypothesesWithUtf8Words,
                          const TUsefulContextForReranker& context, TLogAdapter* logger)
{
    TVector<TAsrHypothesis> asrHypotheses;
    for (const auto& asrHypothesisWords : asrHypothesesWithUtf8Words) {
        asrHypotheses.emplace_back(asrHypothesisWords);
    }

    return DoPickBestHypothesis(asrHypotheses, context, logger);
}

}  // namespace NAlice
