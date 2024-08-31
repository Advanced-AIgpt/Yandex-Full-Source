#include "scenarios_detectors.h"

#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/hash_set.h>

namespace NAlice {

namespace {

using TWord = TUtf16String;

class TPatternsSet {
public:
    virtual bool Apply(const TAsrHypothesis& hypo, int& pos) const = 0;
    virtual ~TPatternsSet() = default;
};

using TConstPatternsSetPtr = TSimpleSharedPtr<const TPatternsSet>;

class TWordsPatterns : public TPatternsSet {
public:
    explicit TWordsPatterns(const TVector<TWord>* patterns, bool isOptional = true)
        : Patterns(patterns)
        , IsOptional(isOptional)
    {
    }

    bool Apply(const TAsrHypothesis& hypo, int& pos) const override {
        if (pos < hypo.Words.ysize() && IsIn(*Patterns, hypo.Words[pos])) {
            ++pos;
            return true;
        }
        return IsOptional;
    }

private:
    const TVector<TWord>* Patterns;
    const bool IsOptional;
};

using TChain = TVector<TWord>;

class TChainsPatterns : public TPatternsSet{
public:
    explicit TChainsPatterns(const TVector<TChain>* patterns, bool isOptional = true)
        : Patterns(patterns)
        , IsOptional(isOptional)
    {
    }

    bool Apply(const TAsrHypothesis& hypo, int& pos) const override {
        const bool somePatternMatches = AnyOf(*Patterns, [&](const auto& pattern) {
            return TryApply(hypo, pos, pattern);
        });
        return somePatternMatches || IsOptional;
    }

private:
    const TVector<TChain>* Patterns;
    const bool IsOptional;

    static bool TryApply(const TAsrHypothesis& hypo, int& pos, const TChain& chain) {
        int curPos = pos;
        for (const auto& word : chain) {
            if (curPos >= hypo.Words.ysize() || hypo.Words[curPos] != word) {
                return false;
            }
            ++curPos;
        }

        pos = curPos;
        return true;
    }
};

class TOneOfTokensPatterns : public TPatternsSet {
public:
    explicit TOneOfTokensPatterns(TVector<TConstPatternsSetPtr>&& patternsOptions, bool isOptional = true)
        : PatternsOptions(patternsOptions)
        , IsOptional(isOptional)
    {
    }

    bool Apply(const TAsrHypothesis& hypo, int& pos) const override {
        return AnyOf(PatternsOptions, [&](const auto& pattern) {
            return pattern->Apply(hypo, pos);
        }) || IsOptional;
    }
private:
    const TVector<TConstPatternsSetPtr> PatternsOptions;
    const bool IsOptional;
};

TConstPatternsSetPtr Make(const TVector<TWord>* patterns, bool isOptional = true) {
    return MakeSimpleShared<const TWordsPatterns>(patterns, isOptional);
}

TConstPatternsSetPtr Make(const TVector<TChain>* patterns, bool isOptional = true) {
    return MakeSimpleShared<const TChainsPatterns>(patterns, isOptional);
}

class TMatcher {
public:
    explicit TMatcher(TVector<TConstPatternsSetPtr>&& tokensOptionsChain)
        : TokensOptionsChain(tokensOptionsChain)
    {
    }

    [[nodiscard]] bool Match(const TAsrHypothesis& hypo) const {
        int pos = 0;
        return AllOf(TokensOptionsChain, [&](const auto& tokensOptionsChain){
            return tokensOptionsChain->Apply(hypo, pos);
        }) && pos == hypo.Words.ysize();
    }

private:
    const TVector<TConstPatternsSetPtr> TokensOptionsChain;
};

const TVector<TWord> AliceWords{
    u"алиса", u"алис", u"яндекс",
};
const TVector<TWord> NumberWords{
    u"один", u"два", u"три", u"четыре", u"пять", u"шесть", u"семь", u"восемь", u"девять", u"десять", u"одиннадцать",
    u"двенадцать", u"тринадцать", u"четырнадцать", u"пятнадцать", u"шестнадцать", u"семнадцать", u"восемнадцать",
    u"девятнадцать", u"двадцать", u"первое", u"первый", u"второе", u"второй", u"третье", u"третий", u"четвертую",
    u"четвёртый", u"пятый", u"пятое", u"шестое", u"шестой", u"седьмое", u"седьмой", u"восьмое", u"восьмой", u"девятое",
    u"девятый", u"десятое", u"десятый",
};
const TVector<TWord> TurnOnWords{
    u"включи", u"включай", u"вруби", u"запусти", u"открой", u"играй", u"проиграй", u"поставь", u"проиграй",
};
const TVector<TWord> VideoWords {
    u"видео", u"видос", u"фильм", u"кино", u"клип",
};
const TVector<TChain> AtNumberWords {
    TChain{u"номер"}, TChain{u"под", u"номером"}
};
const TVector<TWord> ShortCommandsWords {
    u"вверх", u"влево", u"вниз", u"вперёд", u"вправо", u"выключай", u"выключайся", u"выключи", u"выключись",
    u"выключить", u"выруби", u"вырубись", u"вырубить", u"вырубай", u"вырубайся", u"громче", u"дальше", u"довольно",
    u"домой", u"достаточно", u"закройся", u"замолкни", u"замолчи", u"заткнись", u"ладно", u"лайк", u"молодец",
    u"назад", u"налево", u"направо", u"не", u"нет", u"да", u"ок", u"окей", u"ok", u"останови", u"остановись", u"остановить",
    u"отключи", u"отключись", u"отключить", u"отключай", u"отключайся", u"отмена", u"отменить", u"пауза",
    u"перестань", u"погромче", u"потише", u"предыдущая", u"предыдущую", u"продолжай", u"продолжи", u"следующая",
    u"следующую", u"следующий", u"спасибо", u"стоп", u"стой", u"тише", u"умолкни", u"хватит", u"хорош", u"хорошо",
    u"время",
};
const TVector<TWord> MusicMoviePlaybackWords {
    u"музыка", u"музыку", u"трек", u"трэк", u"песню", u"песенку", u"фильм", u"мульт", u"мультик", u"мультфильм",
    u"новости", u"воспроизведение",
};
const TVector<TWord> IoTWords {
    u"ламп", u"лампочк", u"лампул", u"телевизор", u"пылесос", u"кондиционер", u"мультиварк", u"кофеварк", u"чайник",
    u"розетк", u"шторы", u"свет", u"цвет", u"tv", u"освещение", u"люстр", u"кондей", u"устройство", u"утюг", u"посудомойк",
    u"увлажнитель", u"стиральн", u"стиралк", u"телек",
};
const THashSet<TWord> ArithmeticWords {
    u"плюс", u"плюсуй", u"приплюсуй", u"прибавь", u"прибавить", u"прибавляй", u"добавь", u"добавить",
    u"минус", u"вычесть", u"вычти", u"вычитай", u"вычитать", u"отними", u"отнять", u"отнимай",
    u"умножить", u"умножать", u"умножь", u"домножить", u"домножать", u"домножь", u"умножай", u"домножай",
    u"делить", u"разделить", u"поделить", u"дели", u"раздели", u"подели",
    u"степень", u"квадратный", u"корень",
    u"один", u"два", u"три", u"четыре", u"пять", u"шесть", u"семь", u"восемь", u"девять", u"десять", u"одиннадцать",
    u"двенадцать", u"тринадцать", u"четырнадцать", u"пятнадцать", u"шестнадцать", u"семнадцать", u"восемнадцать",
    u"девятнадцать", u"двадцать", u"тридцать", u"сорок", u"пятьдесят", u"шестьдесят", u"семдесят", u"восемдесят", u"девяносто",
    u"сто", u"двести", u"триста", u"четыреста", u"пятьсот", u"шестьсот", u"семьсот", u"восемьсот", u"девятьсот", u"тысяча",
};

const TSimpleSharedPtr<const TPatternsSet> TurnOnOrVideoPatterns =
    MakeSimpleShared<TOneOfTokensPatterns>(TVector<TConstPatternsSetPtr>{
    Make(&TurnOnWords, false),
    Make(&VideoWords, false),
});

const TMatcher ItemSelectorMatcher(TVector<TConstPatternsSetPtr>{
    Make(&AliceWords),
    TurnOnOrVideoPatterns,
    TurnOnOrVideoPatterns,
    Make(&AtNumberWords),
    Make(&NumberWords, /* isOptional = */ false),
    TurnOnOrVideoPatterns,
});

const TMatcher ShortCommandMatcher(TVector<TConstPatternsSetPtr>{
    Make(&AliceWords),
    Make(&ShortCommandsWords, /* isOptional */ false),
    Make(&MusicMoviePlaybackWords),
    Make(&AliceWords),
});

}  // namespace

bool IsIoTCommand(const TAsrHypothesis& asrHypothesis, const TVector<TString>& userIoTScenariosNamesAndTriggers) {
    const auto utterance = asrHypothesis.Utterance();
    const auto utf8Utterance = WideToUTF8(utterance);

    const bool hasIoTWords = AnyOf(IoTWords, [&](const auto& ioTWord) {
        return utterance.Contains(ioTWord);
    });

    return hasIoTWords || IsIn(userIoTScenariosNamesAndTriggers, utf8Utterance);
}

bool IsItemSelectorCommand(const TAsrHypothesis& asrHypothesis) {
    return ItemSelectorMatcher.Match(asrHypothesis);
}

bool IsArithmeticsCommand(const TAsrHypothesis& asrHypothesis) {
    const auto nArithmeticWords = CountIf(asrHypothesis.Words, [&](const auto& word) {
        return IsIn(ArithmeticWords, word);
    });
    return nArithmeticWords >= 2;
}

bool IsShortCommand(const TAsrHypothesis& asrHypothesis) {
    return ShortCommandMatcher.Match(asrHypothesis);
}

}  // namespace NAlice
