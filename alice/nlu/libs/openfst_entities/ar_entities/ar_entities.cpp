#include "ar_entities.h"

#include <contrib/python/pynini/extensions/optimize.h>
#include <contrib/python/pynini/extensions/paths.h>
#include <contrib/python/pynini/extensions/rewrite.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/charset/utf8.h>
#include <util/generic/serialized_enum.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/generic/set.h>

namespace NAlice {

    namespace {

        fst::VectorFst<fst::StdArc> StringToFstInput(const TString& input)
        {
            fst::VectorFst<fst::StdArc> fstInput;
            fst::StringCompiler<fst::StdArc> stringCompiler(fst::TokenType::BYTE);
            stringCompiler(input, &fstInput);
            return fstInput;
        }

        TSpan GetMatchSpan(const TString& fstMatch, const TString& originalText)
        {
            const TString openedCurly = "{";
            const TString closedCurly = "}";
            // get the sharp signs at the head of the match
            const TString head = fstMatch.substr(0, fstMatch.find(openedCurly));
            // get the sharp signs at the tail of the match
            const TString tail = fstMatch.substr(fstMatch.find_last_of(closedCurly) + 1,
                                                 fstMatch.length() - fstMatch.find_last_of(closedCurly) - 1);
            const int headLength = head.length();
            const int tailLength = tail.length();
            // every sharp sign represents one UTF8 caracter in the original text
            const int originalTextUtf8Length = GetNumberOfUTF8Chars(originalText);
            const TStringBuf originalHead = SubstrUTF8(originalText, 0, headLength);
            const TStringBuf originalTail = SubstrUTF8(originalText, originalTextUtf8Length - tailLength, tailLength);
            return {.Begin = originalHead.length(), .End = originalText.length() - originalTail.length()};
        }

        TVector<TString> GetTopMatches(const fst::StdFst& fstModel, const fst::StdFst& fstInput,
                                       unsigned int topMatchCount)
        {
            TVector<std::string> topRewriteStrings;
            TopRewrites(fstInput, fstModel, topMatchCount, &topRewriteStrings, fst::TokenType::BYTE);
            TVector<TString> topRewriteTStrings;
            for (auto topRewrite : topRewriteStrings) {
                topRewriteTStrings.push_back(TString(topRewrite));
            }
            return topRewriteTStrings;
        }

        void UpdateWeekDays(const NJson::TJsonValue& jsonValue, TBitMap<7>& days)
        {
            for (const NJson::TJsonValue& jsonWeekDay : jsonValue[WEEK_DAYS_RESULT].GetArray()) {
                unsigned long long index;
                if (jsonWeekDay.GetUInteger(&index) && index >= TWeekDaysBlock::EDay::Monday + 1 &&
                    index <= TWeekDaysBlock::EDay::Sunday + 1) {
                    days[index - 1] = true;
                }
            }
        }

        void ProcessTime(NJson::TJsonValue& entity) {
            if (entity.Has(DAY_PART)) {
                if (entity[DAY_PART].GetString() == "am" || entity[DAY_PART].GetString() == "pm") {
                    entity.InsertValue(PERIOD, entity[DAY_PART].GetString());
                }
                entity.EraseValue(DAY_PART);
            }
            if (entity.Has(IS_RELATIVE)) {
                if (entity[IS_RELATIVE].GetBoolean()) {
                    if (entity.Has(HOURS)) {
                        entity.InsertValue(HOURS_RELATIVE, NJson::TJsonValue(true));
                    }
                    if (entity.Has(MINUTES)) {
                        entity.InsertValue(MINUTES_RELATIVE, NJson::TJsonValue(true));
                    }
                    if (entity.Has(SECONDS)) {
                        entity.InsertValue(SECONDS_RELATIVE, NJson::TJsonValue(true));
                    }
                }
                entity.EraseValue(IS_RELATIVE);
            }
        }

        void ProcessDate(NJson::TJsonValue& entity)
        {
            if (entity.Has(DAY_PART)) {
                entity.EraseValue(DAY_PART);
            }
            if (entity.Has(IS_RELATIVE)) {
                if (entity[IS_RELATIVE].GetBoolean()) {
                    if (entity.Has(DAYS)) {
                        entity.InsertValue(DAYS_RELATIVE, NJson::TJsonValue(true));
                    }
                    if (entity.Has(MONTHS)) {
                        entity.InsertValue(MONTHS_RELATIVE, NJson::TJsonValue(true));
                    }
                    if (entity.Has(YEARS)) {
                        entity.InsertValue(YEARS_RELATIVE, NJson::TJsonValue(true));
                    }
                    if (entity.Has(WEEKS)) {
                        entity.InsertValue(WEEKS_RELATIVE, NJson::TJsonValue(true));
                    }
                }
                entity.EraseValue(IS_RELATIVE);
            }
        }

        bool IsValidDatetime(const NJson::TJsonValue& entity)
        {
            bool datetimeCheck = NImpl::CheckKeysExist(entity, DATETIME_LIST);
            bool dateCheck = entity.Has(DATE) && NImpl::CheckKeysExist(entity[DATE], DATE_LIST);
            bool timeCheck = entity.Has(TIME) && NImpl::CheckKeysExist(entity[TIME], TIME_LIST);
            bool result = datetimeCheck && (dateCheck || timeCheck);
            return result;
        }

        void MoveChildrenToParent(NJson::TJsonValue& parent, const NJson::TJsonValue& self) {
            for (const auto& [key, value] : self.GetMap()) {
                parent[key] = value;
            }
        }

        void ProcessDatetime(NJson::TJsonValue& entity)
        {
            if (entity.Has(DATE)) {
                ProcessDate(entity[DATE]);
                MoveChildrenToParent(entity, entity[DATE]);
                entity.EraseValue(DATE);
            }
            if (entity.Has(TIME)) {
                ProcessTime(entity[TIME]);
                MoveChildrenToParent(entity, entity[TIME]);
                entity.EraseValue(TIME);
            }
        }

    } // anonymous namespace

    bool TSpan::operator==(const TSpan& rhs) const {
        return this->Begin == rhs.Begin && this->End == rhs.End;
    }

    bool TSpan::operator<(const TSpan& rhs) const {
        return (this->Begin == rhs.Begin) ? (this->End < rhs.End) : (this->Begin < rhs.Begin);
    }

    namespace NImpl {

        NJson::TJsonValue JsonFromFstResult(TString jsonText)
        {
            static const RE2 extraCommaSquare{",\\s*]"};
            static const RE2 extraCommaCurly{",\\s*}"};
            static const TString square = "]";
            static const TString curly = "}";
            RE2::GlobalReplace(&jsonText, extraCommaSquare, square);
            RE2::GlobalReplace(&jsonText, extraCommaCurly, curly);
            return NAlice::JsonFromString(jsonText);
        }

        bool CheckKeysExist(const NJson::TJsonValue& jsonValue, const TVector<TStringBuf>& keys)
        {
            for (TStringBuf key : keys) {
                if (jsonValue.Has(key)) {
                    return true;
                }
            }
            return false;
        }

        size_t GetTokenIndex(const TVector<TSpan>& byteSpansForTokens, const size_t byteIndex) {
            size_t currentTokenIndex = 0;
            size_t currentIndex = 0;
            while (currentTokenIndex < byteSpansForTokens.size() && currentIndex + byteSpansForTokens[currentTokenIndex].End -
                   byteSpansForTokens[currentTokenIndex].Begin < byteIndex) {
                currentIndex += byteSpansForTokens[currentTokenIndex].End - byteSpansForTokens[currentTokenIndex].Begin + 1;
                currentTokenIndex++;
            }
            return currentTokenIndex;
        }

        TSpan GetTokenSpan(const TVector<TSpan>& byteSpansForTokens, const TSpan byteSpan) {
            return {GetTokenIndex(byteSpansForTokens, byteSpan.Begin + 1), GetTokenIndex(byteSpansForTokens, byteSpan.End - 1) + 1};
        }

        TVector<TSpan> GetByteSpansForTokens(const TVector<TString>& tokens) {
            TVector<TSpan> byteSpansForTokens(tokens.size());
            size_t offset = 0;
            for (size_t i = 0; i < tokens.size(); ++i) {
                byteSpansForTokens[i] = {offset, offset + tokens[i].size()};
                offset += tokens[i].size() + 1;
            }
            return byteSpansForTokens;
        }

    } // namespace NImpl

    TEntityBlockBase::TEntityBlockBase(const NJson::TJsonValue& jsonValue)
        : TEntityBlockBase({.Begin = jsonValue[BEGIN].GetInteger(), .End = jsonValue[END].GetInteger()},
                           jsonValue[TEXT].GetString())
    {
    }

    TEntityBlockBase::TEntityBlockBase(const TSpan& span, const TString& text)
        : Span(span)
        , Text(text)
    {
    }

    TSpan TEntityBlockBase::GetSpan() const
    {
        return Span;
    }

    TString TEntityBlockBase::GetText() const
    {
        return Text;
    }

    void TEntityBlockBase::Print(IOutputStream& stream) const
    {
        stream << "begin: " << Span.Begin << " end: " << Span.End << "\n";
        stream << "text: \"" << Text << "\"\n";
        this->PrintImpl(stream);
    }

    TTimeBlock::TTimeBlock(const NJson::TJsonValue& jsonValue)
        : TEntityBlockBase(jsonValue)
        , Hours(jsonValue[CONTENT][HOURS].GetInteger())
        , Minutes(jsonValue[CONTENT][MINUTES].GetInteger())
        , Seconds(jsonValue[CONTENT][SECONDS].GetInteger())
        , IsRelative(jsonValue[IS_RELATIVE].GetBoolean())
        , Repeat(jsonValue[CONTENT][REPEAT].GetBoolean())
        , DayPart(FromString(!jsonValue[CONTENT][PERIOD].GetString().empty() ? jsonValue[CONTENT][PERIOD].GetString()
                                                                    : ToString(EDayPart::Empty)))
    {
    }

    TTimeBlock::TTimeBlock(const TSpan& span, const TString& text, bool isRelative, long long hours, long long minutes,
                           long long seconds, bool repeat, EDayPart dayPart)
        : TEntityBlockBase(span, text)
        , Hours(hours)
        , Minutes(minutes)
        , Seconds(seconds)
        , IsRelative(isRelative)
        , Repeat(repeat)
        , DayPart(dayPart)
    {
    }

    bool TTimeBlock::GetIsRelative() const
    {
        return IsRelative;
    }

    long long TTimeBlock::GetHours() const
    {
        return Hours;
    }

    long long TTimeBlock::GetMinutes() const
    {
        return Minutes;
    }

    long long TTimeBlock::GetSeconds() const
    {
        return Seconds;
    }

    TTimeBlock::EDayPart TTimeBlock::GetDayPart() const
    {
        return DayPart;
    }

    bool TTimeBlock::GetRepeat() const
    {
        return Repeat;
    }

    void TTimeBlock::PrintImpl(IOutputStream& stream) const
    {
        stream << "is relative: " << IsRelative << "\n";
        stream << "hours: " << Hours;
        stream << " minutes: " << Minutes;
        stream << " seconds: " << Seconds;
        stream << " day part: " << DayPart << "\n";
        stream << "repeat: " << Repeat << "\n";
    }

    TDateBlock::TDateBlock(const NJson::TJsonValue& jsonValue)
        : TEntityBlockBase(jsonValue)
        , Days(jsonValue[CONTENT][DAYS].GetInteger())
        , Months(jsonValue[CONTENT][MONTHS].GetInteger())
        , Years(jsonValue[CONTENT][YEARS].GetInteger())
        , IsRelative(jsonValue[IS_RELATIVE].GetBoolean())
        , Repeat(jsonValue[CONTENT][REPEAT].GetBoolean())
    {
    }

    TDateBlock::TDateBlock(const TSpan& span, const TString& text, bool isRelative, long long days, long long months,
                           long long years, bool repeat)
        : TEntityBlockBase(span, text)
        , Days(days)
        , Months(months)
        , Years(years)
        , IsRelative(isRelative)
        , Repeat(repeat)
    {
    }

    bool TDateBlock::GetIsRelative() const
    {
        return IsRelative;
    }

    long long TDateBlock::GetDays() const
    {
        return Days;
    }

    long long TDateBlock::GetMonths() const
    {
        return Months;
    }

    long long TDateBlock::GetYears() const
    {
        return Years;
    }

    bool TDateBlock::GetRepeat() const
    {
        return Repeat;
    }

    void TDateBlock::PrintImpl(IOutputStream& stream) const
    {
        stream << "is relative: " << IsRelative << "\n";
        stream << "days: " << Days;
        stream << " months: " << Months;
        stream << " years: " << Years << "\n";
        stream << "repeat: " << Repeat << "\n";
    }

    TWeekDaysBlock::TWeekDaysBlock(const NJson::TJsonValue& jsonValue)
        : TEntityBlockBase(jsonValue)
        , Repeat(jsonValue[CONTENT][REPEAT].GetBoolean())
    {
        UpdateWeekDays(jsonValue[CONTENT], Days);
    }

    TWeekDaysBlock::TWeekDaysBlock(const TSpan& span, const TString& text, const TBitMap<7>& days, bool repeat)
        : TEntityBlockBase(span, text)
        , Days(days)
        , Repeat(repeat)
    {
    }

    bool TWeekDaysBlock::GetRepeat() const
    {
        return Repeat;
    }

    TBitMap<7> TWeekDaysBlock::GetDays() const
    {
        return Days;
    }

    void TWeekDaysBlock::PrintImpl(IOutputStream& stream) const
    {
        stream << "repeat: " << Repeat << "\n";
        stream << "days:";
        const NEnumSerializationRuntime::TMappedArrayView<EDay> eDays = GetEnumAllValues<EDay>();
        for (auto eDay : eDays) {
            if (Days[eDay]) {
                stream << " " << eDay << ",";
            }
        }
        stream << "\n";
    }

    TNumberBlock::TNumberBlock(const NJson::TJsonValue& jsonValue)
        : TEntityBlockBase(jsonValue)
        , Number(jsonValue[CONTENT].GetInteger())
    {
    }

    TNumberBlock::TNumberBlock(const TSpan& span, const TString& text, long long number)
        : TEntityBlockBase(span, text)
        , Number(number)
    {
    }

    long long TNumberBlock::GetNumber() const
    {
        return Number;
    }

    void TNumberBlock::PrintImpl(IOutputStream& stream) const
    {
        stream << "number: " << Number << "\n";
    }

    TFloatBlock::TFloatBlock(const NJson::TJsonValue& jsonValue)
        : TEntityBlockBase(jsonValue)
        , FloatValue(jsonValue[CONTENT].GetDouble())
    {
    }

    TFloatBlock::TFloatBlock(const TSpan& span, const TString& text, double floatValue)
        : TEntityBlockBase(span, text)
        , FloatValue(floatValue)
    {
    }

    double TFloatBlock::GetFloatValue() const
    {
        return FloatValue;
    }

    void TFloatBlock::PrintImpl(IOutputStream& stream) const
    {
        stream << "float value: " << FloatValue << "\n";
    }

    TEntityParser::TEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount)
        : Reader(fst::FarReader<fst::StdArc>::Open(farPath.data()))
        , MaxHypothesisCount(maxHypothesisCount)
    {
        Y_ENSURE(Reader);
    }

    TVector<NJson::TJsonValue> TEntityParser::GetJsonEntitiesWithRawByteSpans(const TString& input) const
    {
        TVector<NJson::TJsonValue> entities;
        const fst::StdFst& fstInput = StringToFstInput(input);
        const TVector<TString> fstMatches = GetTopMatches(GetFst(), fstInput, MaxHypothesisCount);
        for (TString fstMatch : fstMatches) {
            const TSpan& span = GetMatchSpan(fstMatch, input);
            // remove sharp signs from the beginning and the end of the match
            static const RE2 endsWithSharp{"#*$"};
            static const RE2 startsWithSharp{"^#*"};
            RE2::GlobalReplace(&fstMatch, startsWithSharp, "");
            RE2::GlobalReplace(&fstMatch, endsWithSharp, "");
            NJson::TJsonValue entity;
            entity[CONTENT] = NImpl::JsonFromFstResult(fstMatch);
            if (!this->IsValid(entity[CONTENT])) {
                continue;
            }
            const TString matchText = input.substr(span.Begin, span.End - span.Begin);
            entity.InsertValue(BEGIN, NJson::TJsonValue(span.Begin));
            entity.InsertValue(END, NJson::TJsonValue(span.End));
            entity.InsertValue(TEXT, NJson::TJsonValue(matchText));
            this->Process(entity);
            entities.push_back(entity);
        }
        return entities;
    }

    TVector<NJson::TJsonValue> TEntityParser::GetJsonEntities(const TString& input) const
    {
        TVector<NJson::TJsonValue> entities = TEntityParser::GetJsonEntitiesWithRawByteSpans(input);
        const TVector<TString> tokens = StringSplitter(input).Split(' ');
        const TVector<TSpan> byteSpansForTokens = NImpl::GetByteSpansForTokens(tokens);
        for (NJson::TJsonValue& entity : entities) {
            const TSpan byteSpan = {entity[BEGIN].GetUInteger(), entity[END].GetUInteger()};
            const TSpan tokenSpan = NImpl::GetTokenSpan(byteSpansForTokens, byteSpan);
            const TString matchText = JoinRange(" ", tokens.begin() + tokenSpan.Begin, tokens.begin() + tokenSpan.End);
            entity.InsertValue(BEGIN, NJson::TJsonValue(tokenSpan.Begin));
            entity.InsertValue(END, NJson::TJsonValue(tokenSpan.End));
            entity.InsertValue(TEXT, NJson::TJsonValue(matchText));
        }
        return this->PostProcess(entities);
    }

    TVector<NJson::TJsonValue> TEntityParser::PostProcess(const TVector<NJson::TJsonValue>& entities) const {
        TVector<NJson::TJsonValue> result;
        TSet<TSpan> usedSpans;
        for (const NJson::TJsonValue& entity : entities) {
            const TSpan span = {entity[BEGIN].GetUInteger(), entity[END].GetUInteger()};
            if (!usedSpans.contains(span)) {
                usedSpans.insert(span);
                result.emplace_back(entity);
            }
        }
        return result;
    }

    const fst::StdFst& TEntityParser::GetFst() const
    {
        Y_ENSURE(Reader->GetFst());
        return *Reader->GetFst();
    }

    bool TEntityParser::IsValid(const NJson::TJsonValue& entity) const
    {
        return true;
    }

    void TEntityParser::Process(NJson::TJsonValue& entity) const
    {
    }

    TTimeEntityParser::TTimeEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount)
        : TEntityParser(farPath, maxHypothesisCount)
    {
    }

    bool TTimeEntityParser::IsValid(const NJson::TJsonValue& entity) const
    {
        return NImpl::CheckKeysExist(entity, TIME_LIST);
    }

    void TTimeEntityParser::Process(NJson::TJsonValue& entity) const
    {
        if (entity[CONTENT].Has(IS_RELATIVE)) {
            entity[IS_RELATIVE] = entity[CONTENT][IS_RELATIVE];
        }
        ProcessTime(entity[CONTENT]);
    }

    TDateEntityParser::TDateEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount)
        : TEntityParser(farPath, maxHypothesisCount)
    {
    }

    bool TDateEntityParser::IsValid(const NJson::TJsonValue& entity) const
    {
        return NImpl::CheckKeysExist(entity, DATE_LIST);
    }

    void TDateEntityParser::Process(NJson::TJsonValue& entity) const
    {
        if (entity[CONTENT].Has(IS_RELATIVE)) {
            entity[IS_RELATIVE] = entity[CONTENT][IS_RELATIVE];
        }
        ProcessDate(entity[CONTENT]);
    }

    TWeekDaysEntityParser::TWeekDaysEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount)
        : TEntityParser(farPath, maxHypothesisCount)
    {
    }

    void TWeekDaysEntityParser::UpdateWeekDaysInterval(NJson::TJsonValue& jsonValue) const
    {
        TVector<int> weekDays;
        if (jsonValue.Has(WEEK_DAYS)) {
            for (const NJson::TJsonValue& day : jsonValue[WEEK_DAYS].GetArray()) {
                weekDays.push_back(day.GetInteger() + 1);
            }
            jsonValue.EraseValue(WEEK_DAYS);
        }

        int start = -1;
        int end = -1;

        if (jsonValue.Has(WEEK_DAYS_START)) {
            start = jsonValue[WEEK_DAYS_START].GetInteger();
            jsonValue.EraseValue(WEEK_DAYS_START);
        }
        if (jsonValue.Has(WEEK_DAYS_END)) {
            end = jsonValue[WEEK_DAYS_END].GetInteger();
            jsonValue.EraseValue(WEEK_DAYS_END);
        }

        if (0 <= start && start < 7 && 0 <= end && end < 7) {
            int day = start;
            do {
                weekDays.push_back(day + 1);
                day = (day + 1) % 7;
            }
            while (day != (end + 1) % 7);
        }

        std::sort(weekDays.begin(), weekDays.end());
        weekDays.resize(std::unique(weekDays.begin(), weekDays.end()) - weekDays.begin());

        NJson::TJsonArray weekDaysResult;
        for (int day : weekDays) {
            weekDaysResult.AppendValue(NJson::TJsonValue(day));
        }
        jsonValue.InsertValue(WEEK_DAYS_RESULT, NJson::TJsonArray(weekDaysResult));
    }

    void TWeekDaysEntityParser::RemoveExcludedweekDays(NJson::TJsonValue& jsonValue) const
    {
        const NJson::TJsonValue::TArray excludedWeekDays = jsonValue[EXCEPT_WEEK_DAYS].GetArray();
        if (jsonValue.Has(EXCEPT_WEEK_DAYS)) {
            jsonValue.EraseValue(EXCEPT_WEEK_DAYS);
        } else {
            return;
        }

        NJson::TJsonValue::TArray weekDays = jsonValue[WEEK_DAYS_RESULT].GetArray();
        for (const NJson::TJsonValue& excludedWeekDay : excludedWeekDays) {
            long long dayToExclude;
            if (excludedWeekDay.GetInteger(&dayToExclude)) {
                size_t index = 0;
                for (const NJson::TJsonValue& weekDay : weekDays) {
                    long long day;
                    if (weekDay.GetInteger(&day) && dayToExclude == day - 1) {
                        weekDays.erase(weekDays.begin() + index);
                        break;
                    }
                    index++;
                }
            }
        }
        jsonValue[WEEK_DAYS_RESULT].GetArraySafe() = weekDays;
    }

    bool TWeekDaysEntityParser::IsValid(const NJson::TJsonValue& entity) const
    {
        return NImpl::CheckKeysExist(entity, WEEK_DAYS_LIST);
    }

    void TWeekDaysEntityParser::Process(NJson::TJsonValue& entity) const
    {
        UpdateWeekDaysInterval(entity[CONTENT]);
        RemoveExcludedweekDays(entity[CONTENT]);
    }

    TNumberEntityParser::TNumberEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount)
        : TEntityParser(farPath, maxHypothesisCount)
    {
    }

    bool TNumberEntityParser::IsValid(const NJson::TJsonValue& entity) const
    {
        return NImpl::CheckKeysExist(entity, NUMBER_LIST);
    }

    void TNumberEntityParser::Process(NJson::TJsonValue& entity) const
    {
        entity[CONTENT] = entity[CONTENT][NUMBER];
    }

    TFloatEntityParser::TFloatEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount)
        : TEntityParser(farPath, maxHypothesisCount)
    {
    }

    bool TFloatEntityParser::IsValid(const NJson::TJsonValue& entity) const
    {
        return NImpl::CheckKeysExist(entity, FLOAT_LIST);
    }

    void TFloatEntityParser::Process(NJson::TJsonValue& entity) const
    {
        double result = 0;
        if (entity[CONTENT].Has(NUMBER)) {
            result += entity[CONTENT][NUMBER].GetInteger();
            entity[CONTENT].EraseValue(NUMBER);
        }
        double floatValue = 0;
        if (entity[CONTENT].Has(POWER_DIVISOR) or entity[CONTENT].Has(NUMBER_DIVISOR)) {
            floatValue = 1;
        }
        if (entity[CONTENT].Has(MULTIPLIER)) {
            floatValue = entity[CONTENT][MULTIPLIER].GetInteger();
            entity[CONTENT].EraseValue(MULTIPLIER);
        }
        if (entity[CONTENT].Has(NUMBER_DIVISOR) and entity[CONTENT][NUMBER_DIVISOR].GetInteger() != 0) {
            floatValue /= entity[CONTENT][NUMBER_DIVISOR].GetInteger();
            entity[CONTENT].EraseValue(NUMBER_DIVISOR);
        }
        if (entity[CONTENT].Has(POWER_DIVISOR)) {
            if (entity[CONTENT][POWER_DIVISOR].GetInteger() == -1) {
                int divisor = 1;
                if (floatValue < 0) {
                    floatValue = -floatValue;
                }
                while ((int)(floatValue / divisor) > 0) {
                    divisor *= 10;
                }
                floatValue /= divisor;
            }
            else {
                floatValue /= pow(10, entity[CONTENT][POWER_DIVISOR].GetInteger());
            }
            entity[CONTENT].EraseValue(POWER_DIVISOR);
        }
        result += floatValue;
        entity[CONTENT] = result;
    }

    TDatetimeEntityParser::TDatetimeEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount)
        : TEntityParser(farPath, maxHypothesisCount)
    {
    }

    bool TDatetimeEntityParser::IsValid(const NJson::TJsonValue& entity) const
    {
        return IsValidDatetime(entity);
    }

    void TDatetimeEntityParser::Process(NJson::TJsonValue& entity) const
    {
        ProcessDatetime(entity[CONTENT]);
    }

    TDatetimeRangeEntityParser::TDatetimeRangeEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount)
        : TEntityParser(farPath, maxHypothesisCount)
    {
    }

    bool TDatetimeRangeEntityParser::IsValid(const NJson::TJsonValue& entity) const
    {
        bool datetimeRangeCheck = NImpl::CheckKeysExist(entity, DATETIME_RANGE_LIST);
        bool startCheck = entity.Has(DATETIME_RANGE_START) && IsValidDatetime(entity[DATETIME_RANGE_START]);
        bool endCheck = entity.Has(DATETIME_RANGE_END) && IsValidDatetime(entity[DATETIME_RANGE_END]);
        bool result = datetimeRangeCheck && (startCheck || endCheck);
        return result;
    }

    void TDatetimeRangeEntityParser::Process(NJson::TJsonValue& entity) const
    {
        if (entity[CONTENT].Has(DATETIME_RANGE_START)) {
            ProcessDatetime(entity[CONTENT][DATETIME_RANGE_START]);
        }
        if (entity[CONTENT].Has(DATETIME_RANGE_END)) {
            ProcessDatetime(entity[CONTENT][DATETIME_RANGE_END]);
        }
    }

} // namespace NAlice
