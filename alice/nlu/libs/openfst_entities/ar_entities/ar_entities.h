#pragma once

#include "json_keys.h"

#include <alice/library/json/json.h>

#include <fst/extensions/far/far.h>
#include <fst/fstlib.h>

#include <util/generic/bitmap.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NAlice {

    struct TSpan {
        size_t Begin = 0;
        size_t End = 0;
        bool operator==(const TSpan& rhs) const;
        bool operator<(const TSpan& rhs) const;
    };

    namespace NImpl {

        NJson::TJsonValue JsonFromFstResult(TString jsonText);
        bool CheckKeysExist(const NJson::TJsonValue& jsonValue, const TVector<TStringBuf>& keys);
        size_t GetTokenIndex(const TVector<TSpan>& byteSpansForTokens, const size_t byteIndex);
        TSpan GetTokenSpan(const TVector<TSpan>& byteSpansForTokens, const TSpan byteSpan);
        TVector<TSpan> GetByteSpansForTokens(const TVector<TString>& tokens);
        
    } // namespace NImpl

    class TEntityBlockBase {
    public:
        explicit TEntityBlockBase(const NJson::TJsonValue& jsonValue);
        TEntityBlockBase(const TSpan& span, const TString& text);
        virtual ~TEntityBlockBase() = default;
        void Print(IOutputStream& stream) const;
        TSpan GetSpan() const;
        TString GetText() const;

    private:
        virtual void PrintImpl(IOutputStream& stream) const = 0;

    private:
        TSpan Span;
        TString Text;
    };

    class TTimeBlock : public TEntityBlockBase {
    public:
        enum EDayPart
        {
            Empty = 0 /* "empty" */,
            Am = 1 /* "am" */,
            Pm = 2 /* "pm" */,
            Ambiguous = 3 /* "ambiguous" */
        };

    public:
        explicit TTimeBlock(const NJson::TJsonValue& jsonValue);
        TTimeBlock(const TSpan& span, const TString& text, bool isRelative, long long hours, long long minutes,
                   long long seconds, bool repeat, EDayPart dayPart);
        bool GetIsRelative() const;
        long long GetHours() const;
        long long GetMinutes() const;
        long long GetSeconds() const;
        EDayPart GetDayPart() const;
        bool GetRepeat() const;

    private:
        void PrintImpl(IOutputStream& stream) const override;

    private:
        bool IsRelative = false;
        long long Hours = 0;
        long long Minutes = 0;
        long long Seconds = 0;
        EDayPart DayPart = EDayPart::Empty; // am, pm, empty if not specified and ambiguous otherwise
        bool Repeat = false;
    };

    class TDateBlock : public TEntityBlockBase {
    public:
        explicit TDateBlock(const NJson::TJsonValue& jsonValue);
        TDateBlock(const TSpan& span, const TString& text, bool isRelative, long long days, long long months,
                   long long years, bool repeat);
        bool GetIsRelative() const;
        long long GetDays() const;
        long long GetMonths() const;
        long long GetYears() const;
        bool GetRepeat() const;

    private:
        void PrintImpl(IOutputStream& stream) const override;

    private:
        bool IsRelative = false;
        long long Days = 0;
        long long Months = 0;
        long long Years = 0;
        bool Repeat = false;
    };

    class TWeekDaysBlock : public TEntityBlockBase {
    public:
        enum EDay
        {
            Monday = 0,
            Tuesday = 1,
            Wednesday = 2,
            Thursday = 3,
            Friday = 4,
            Saturday = 5,
            Sunday = 6
        };

    public:
        explicit TWeekDaysBlock(const NJson::TJsonValue& jsonValue);
        TWeekDaysBlock(const TSpan& span, const TString& text, const TBitMap<7>& days, bool repeat);
        bool GetRepeat() const;
        TBitMap<7> GetDays() const;

    private:
        void PrintImpl(IOutputStream& stream) const override;

    private:
        TBitMap<7> Days;
        bool Repeat = false;
    };

    class TNumberBlock : public TEntityBlockBase {
    public:
        explicit TNumberBlock(const NJson::TJsonValue& jsonValue);
        TNumberBlock(const TSpan& span, const TString& text, long long number);
        long long GetNumber() const;

    private:
        void PrintImpl(IOutputStream& stream) const override;

    private:
        long long Number = 0;
    };

    class TFloatBlock : public TEntityBlockBase {
    public:
        explicit TFloatBlock(const NJson::TJsonValue& jsonValue);
        TFloatBlock(const TSpan& span, const TString& text, double floatValue);
        double GetFloatValue() const;

    private:
        void PrintImpl(IOutputStream& stream) const override;

    private:
        double FloatValue = 0;
    };

    class TEntityParser {
    public:
        explicit TEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount = 10);
        TVector<NJson::TJsonValue> GetJsonEntitiesWithRawByteSpans(const TString& input) const;
        TVector<NJson::TJsonValue> GetJsonEntities(const TString& input) const;

    private:
        virtual bool IsValid(const NJson::TJsonValue& entity) const;
        const fst::StdFst& GetFst() const;
        virtual void Process(NJson::TJsonValue& entity) const;
        virtual TVector<NJson::TJsonValue> PostProcess(const TVector<NJson::TJsonValue>& entities) const;

    private:
        THolder<fst::FarReader<fst::StdArc>> Reader;
        unsigned int MaxHypothesisCount = 10;
    };

    class TTimeEntityParser : public TEntityParser {
    public:
        explicit TTimeEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount = 10);

    private:
        bool IsValid(const NJson::TJsonValue& entity) const override;
        void Process(NJson::TJsonValue& entity) const override;
    };

    class TDateEntityParser : public TEntityParser {
    public:
        explicit TDateEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount = 10);

    private:
        bool IsValid(const NJson::TJsonValue& entity) const override;
        void Process(NJson::TJsonValue& entity) const override;
    };

    class TWeekDaysEntityParser : public TEntityParser {
    public:
        explicit TWeekDaysEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount = 10);

    private:
        void UpdateWeekDaysInterval(NJson::TJsonValue& jsonValue) const;
        void RemoveExcludedweekDays(NJson::TJsonValue& jsonValue) const;
        bool IsValid(const NJson::TJsonValue& entity) const override;
        void Process(NJson::TJsonValue& entity) const override;
    };

    class TNumberEntityParser : public TEntityParser {
    public:
        explicit TNumberEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount = 10);

    private:
        bool IsValid(const NJson::TJsonValue& entity) const override;
        void Process(NJson::TJsonValue& entity) const override;
    };

    class TFloatEntityParser : public TEntityParser {
    public:
        explicit TFloatEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount = 10);

    private:
        bool IsValid(const NJson::TJsonValue& entity) const override;
        void Process(NJson::TJsonValue& entity) const override;
    };

    class TDatetimeEntityParser : public TEntityParser {
    public:
        explicit TDatetimeEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount = 10);

    private:
        bool IsValid(const NJson::TJsonValue& entity) const override;
        void Process(NJson::TJsonValue& entity) const override;
    };

    class TDatetimeRangeEntityParser : public TEntityParser {
    public:
        explicit TDatetimeRangeEntityParser(TStringBuf farPath, unsigned int maxHypothesisCount = 10);

    private:
        bool IsValid(const NJson::TJsonValue& entity) const override;
        void Process(NJson::TJsonValue& entity) const override;
    };

} // namespace NAlice
