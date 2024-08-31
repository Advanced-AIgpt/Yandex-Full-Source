#pragma once

#include <alice/nlg/library/runtime_api/env.h>

namespace NAlice::NNlg::NBuiltins {

// Jinja2's filters
TValue Abs(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Attr(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& name);
TValue Capitalize(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue CapitalizeFirst(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Decapitalize(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue DecapitalizeFirst(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Default(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& defValue,
               const TValue& boolean);
TValue First(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Float(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& defValue);
TValue FormatWeekday(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue GetItem(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& key,
               const TValue& defValue);
TValue Div2Escape(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue HtmlEscape(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue HumanMonth(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& grams);
TValue Int(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& defValue);
TValue Inflect(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& cases,
               const TValue& fio);
TValue Join(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& delimiter,
            const TValue& attribute);
TValue Last(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Length(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue List(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Lower(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Map(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& filter,
           const TValue& attribute);
TValue Max(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Min(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue NumberOfReadableTokens(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Random(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Replace(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& from,
               const TValue& to);
TValue Round(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& precision,
             const TValue& method);
TValue SplitBigNumber(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue String(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue ToJson(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Trim(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TtsDomain(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& domain);
TValue Upper(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue Urlencode(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);

// Jinja2's methods
TValue DictGet(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& key,
               const TValue& defValue);
TValue DictItems(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue DictKeys(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue DictUpdate(const TCallCtx& ctx, const TGlobalsChain* globals, TValue target, const TValue& other);
TValue DictValues(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue ListAppend(const TCallCtx& ctx, const TGlobalsChain* globals, TValue target, const TValue& item);
TValue StrEndsWith(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& suffix);
TValue StrJoin(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& items);
TValue StrLower(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue StrLstrip(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& chars);
TValue StrReplace(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& substr,
                  const TValue& repl);
TValue StrRstrip(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& chars);
TValue StrSplit(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& sep,
                const TValue& num);
TValue StrStartsWith(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& prefix);
TValue StrStrip(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& chars);
TValue StrUpper(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);

// Jinja2's tests
TValue TestDefined(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestUndefined(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestDivisibleBy(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& num);
TValue TestEven(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestOdd(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestEq(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& other);
TValue TestGe(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& other);
TValue TestGt(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& other);
TValue TestIn(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& other);
TValue TestLe(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& other);
TValue TestLt(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& other);
TValue TestNe(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& other);
TValue TestIterable(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestLower(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestUpper(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestMapping(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestNone(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestNumber(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestSequence(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue TestString(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);

// NLG-specific filters
TValue CityPrepcase(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& value);
TValue Emojize(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& value);
TValue MusicTitleShorten(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& value);
TValue TrimWithEllipsis(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& value,
                        const TValue& widthLimit);
TValue OnlyText(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& value);
TValue OnlyVoice(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& value);
TValue Pluralize(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& number, const TValue& inflCase);
TValue Singularize(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& number);
TValue HumanTimeRaw(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue HumanTime(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& timezone = TValue::None());
TValue HumanDate(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& timezone = TValue::None());
TValue HumanDayRel(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& timezone = TValue::None(),
                   const TValue& mockedTime = TValue::None());
TValue IsHumanDayRel(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& timezone = TValue::None(),
                   const TValue& mockedTime = TValue::None());

// NLG-specific functions
TValue ClientActionDirective(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& name,
                             const TValue& payload, const TValue& type, const TValue& subName);
TValue CreateDateSafe(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& year, const TValue& month,
                      const TValue& day);
TValue CreateDateWeekend(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& dayofweek);
TValue Datetime(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& year, const TValue& month, const TValue& day,
                const TValue& hour = TValue::Integer(0), const TValue& minute = TValue::Integer(0),
                const TValue& second = TValue::Integer(0), const TValue& microsecond = TValue::Integer(0));
TValue TimestampToDatetime(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& timestamp,
                           const TValue& timezone = TValue::String("UTC"));
TValue DatetimeStrftime(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& datetimeFormat);
TValue DatetimeStrptime(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& dateString, const TValue& dateFormat);
TValue ParseDt(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue DatetimeIsoweekday(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target);
TValue ParseTz(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& timezoneFormat);
TValue Localize(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& datetime);
TValue PluralizeTag(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& inflCase);
TValue Randuniform(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& from, const TValue& to);
TValue ServerActionDirective(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& name,
                             const TValue& payload, const TValue& type, const TValue& ignoreAnswer);
TValue AddHours(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& datetime, const TValue& hours = TValue::Integer(0));
TValue CeilSeconds(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& time_units, const TValue& aggressive);
TValue NormalizeTimeUnits(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& timeUnits);
TValue RenderWeekdayType(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& weekday);
TValue RenderWeekdaySimple(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& weekday);
TValue RenderDatetimeRaw(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& dt);
TValue RenderDateWithOnPreposition(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& dt);
TValue RenderUnitsTime(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& units, const TValue& cases);
TValue TimeFormat(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& time, const TValue& cases);

// NLG-specific macros
void Macro_ChooseLine(const TCallCtx& ctx, const TCaller* caller, const TGlobalsChain* globals, IOutputStream& out);

} // namespace NAlice::NNlg::NBuiltins
