//
// Scenario GET_DATE
//

#include "get_date.h"
#include "get_date_frames.h"

#include <alice/hollywood/library/scenarios/get_date/nlg/register.h>
#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>
#include <alice/hollywood/library/scenarios/get_date/slot_utils/slot_utils.h>
#include <alice/hollywood/library/scenarios/get_date/slot_utils/calendar_utils.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/protos/data/entities/datetime.pb.h>

#include <library/cpp/geobase/timezone_getter.hpp>
#include <library/cpp/timezone_conversion/civil.h>

namespace NAlice::NHollywoodFw::NGetDate {

HW_REGISTER(TGetDateScenario);

namespace {

//
// Move information from semantic frames to TGetDateState structure
// Note ellipsis semantic frame doesn't have SLOT_CHECK_DATE, so checkDateSlot and checkDateParser are used for primary frame only
// @return false if slot info is irrelevant
//
bool MergeSemanticFrames(TGetDateSceneArgs& args, const TGetDateFrame& frame) {
    // Stage 1. Collect all date/time slots
    int pos = 0;
    for (const auto& it : frame.CalerdarDates.Value) {
        if (args.GetDate().size() > pos) {
            // Replace slot
            *(args.MutableDate()->Mutable(pos)) = it.GetAsProtoDatetime();
        } else {
            // Add slot
            args.MutableDate()->Add(it.GetAsProtoDatetime());
        }
        pos++;
    }
    // Stage 2. Check and add numeric slot (convert it to date)
    if (frame.NumericDates.Defined()) {
        if (*frame.NumericDates.Value < 1 || *frame.NumericDates.Value > 31) {
            // This request can't be classified as a valid date
            return false;
        }
        TSysDatetimeValue dateFromNumeric;
        dateFromNumeric.MutableDateValue()->SetDays(*frame.NumericDates.Value);
        args.MutableDate()->Add(std::move(dateFromNumeric));
    }
    // Add other slots
    if (frame.Tense.Defined()) {
        args.SetTense(*frame.Tense.Value);
    }
    switch (frame.QueryTarget.Value.size()) {
        case 0:
            // $QueryTarget is not set
            break;
        case 1:
            // $QueryTarget is set one time only
            args.SetQueryTarget(frame.QueryTarget.Value[0]);
            break;
        default:
            // 2 and more $QueryTarget
            args.SetQueryTarget("all_with_year");
    }
    if (frame.Location.Defined()) {
        args.SetWhere(*frame.Location.Value);
        args.SetWhereType(frame.Location.GetType());
    }
    return true;
}

//
// Try to extract and use previous frame state (only if Ellipsis frame exists)
// Return false, if data are obsolete/new session/doesn't exist
//
bool ExtractPreviousFrameState(TGetDateState& previousFrameState,
                               const TRunRequest& runRequest,
                               const TStorage& storage)
{
    switch (storage.GetScenarioState(previousFrameState, std::chrono::minutes(5))) {
        case TStorage::EScenarioStateResult::Absent:
            LOG_INFO(runRequest.Debug().Logger()) << "Frame state doesn't contain TGetDateState proto";
            return false;
        case TStorage::EScenarioStateResult::Expired:
            LOG_INFO(runRequest.Debug().Logger()) << "Frame state is expired";
            return false;
        case TStorage::EScenarioStateResult::NewSession:
            LOG_INFO(runRequest.Debug().Logger()) << "New session flag set";
            return false;
        case TStorage::EScenarioStateResult::Present:
            break;
    }
    LOG_DEBUG(runRequest.Debug().Logger()) << "Previous state is valid, continue working with ellipsis";
    return true;
}

//
// Dump information about current frame state, includes additional datetimeparse information
//
void DebugDump(const TGetDateSceneArgs& args, TRTLogger& logger, const TStringBuf prefix) {
    LOG_DEBUG(logger) << prefix;
    for (const auto& it : args.GetDate()) {
        LOG_DEBUG(logger) << "CalendarDate: " << it.AsJSON();
        TMaybe<TSysDatetimeParser> dateInfo = TSysDatetimeParser::Parse(it);
        if (dateInfo) {
            LOG_DEBUG(logger) << "CalendarDate parse information: parse_info: " << static_cast<int>(dateInfo->GetParseInfo())
                              << "; content: " << static_cast<int>(dateInfo->GetParseContent());
        }
    }
    LOG_DEBUG(logger) << "Tense: " << args.GetTense();
    LOG_DEBUG(logger) << "Location: " << args.GetWhereType() << ":" << args.GetWhere();
    LOG_DEBUG(logger) << "Target: " << args.GetQueryTarget();
}

} // anonymous namespace


TGetDateScenario::TGetDateScenario()
    : TScenario(NProductScenarios::GET_DATE)
{
    Register(&TGetDateScenario::Dispatch);
    RegisterScene<TGetDateScene>([this]() {
        RegisterSceneFn(&TGetDateScene::Main);
    });
    RegisterRenderer(&TGetDateScenario::Render);

    // Additional functions
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NGetDate::NNlg::RegisterAll);
    SetApphostGraph(ScenarioRequest() >> NodeRun() >> NodeMain() >> ScenarioResponse());
}

TRetScene TGetDateScenario::Dispatch(const TRunRequest& runRequest,
                                     const TStorage& storage,
                                     const TSource&) const {
    const TGetDateFrame frameGetDate(runRequest.Input(), FRAME_GET_DATE);
    const TGetDateFrame frameGetDateEllipsis(runRequest.Input(), FRAME_GET_DATE_ELLIPSIS);

    if (!frameGetDate.Defined() && !frameGetDateEllipsis.Defined()) {
        // Unexpected case - both semantic frames are unavailable
        // Probably MM config was changed
        return TReturnValueRenderIrrelevant("get_date", "error");
    }
    const TStringBuf selectedFrame = frameGetDate.Defined() ? FRAME_GET_DATE : FRAME_GET_DATE_ELLIPSIS;

    TGetDateState previousFrameState;
    // Extract previousFrameState (only for ellipsis requests)
    if (!frameGetDate.Defined()) {
        if (!ExtractPreviousFrameState(previousFrameState, runRequest, storage)) {
            return TReturnValueRenderIrrelevant("get_date", "error");
        }
    }

    //
    // Now we have following sources to expand requests
    // frameDate - semantic frame for primary requests (optional)
    // frameDateEllipsis - semantic frame with ellipsis (optional)
    // previousFrameState - state from previous frame (optional)
    //
    // Merge all data together in following order:
    //
    // - previousFrameState (initial)
    // - frameDate (if present)
    // - frameDateEllipsis (if present)
    //
    TGetDateSceneArgs args;
    if (previousFrameState.HasTense()) {
        args.SetTense(previousFrameState.GetTense().GetStringValue());
    }
    if (previousFrameState.HasQueryTarget()) {
        args.SetQueryTarget(previousFrameState.GetQueryTarget().GetStringValue());
    }
    if (previousFrameState.HasWhere()) {
        args.SetWhere(previousFrameState.GetWhere().GetStringValue());
    }
    if (previousFrameState.HasWhereType()) {
        args.SetWhereType(previousFrameState.GetWhereType().GetStringValue());
    }
    *args.MutableDate() = previousFrameState.GetDate();

    bool mergeResult = true;
    if (frameGetDate.Defined()) {
        mergeResult = mergeResult && MergeSemanticFrames(args, frameGetDate);
    }
    if (frameGetDateEllipsis.Defined()) {
        mergeResult = mergeResult && MergeSemanticFrames(args, frameGetDateEllipsis);
    }
    if (!mergeResult) {
        // MergeSemanticFrames() failed to extract data (sys.num < 1 || sys.num > 31)
        return TReturnValueRenderIrrelevant("get_date", "wrong_date");
    }
    DebugDump(args, runRequest.Debug().Logger(), "Common getdate info:");
    return TReturnValueScene<TGetDateScene>(args, TString(selectedFrame));
}

TRetResponse TGetDateScenario::Render(const TGetDateRenderProto& args, TRender& render) {
    render.MakeComplexVar("SourceDate", NJson::ReadJsonFastTree(args.GetSourceDate()));
    render.MakeComplexVar("CityPreparse", NJson::ReadJsonFastTree(args.GetCityPreparse()));
    render.CreateFromNlg("get_date", args.GetPhrase(), args);
    render.AddEllipsisFrame(FRAME_GET_DATE_ELLIPSIS);

    // Add suggests
    render.AddSuggestion("get_date", "suggest_tomorrow");
    render.AddSuggestion("get_date", "suggest_today");
    render.AddSuggestion("get_date", "suggest_whatyoucan");
    render.AddSuggestion("get_date", "suggest_holiday");
    render.AddSuggestion("get_date", "suggest_history");
    render.AddSuggestion("get_date", "suggest_news");
    return TReturnValueSuccess();
}

TGetDateScene::TGetDateScene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_DEFAULT)
{
}

TRetMain TGetDateScene::Main(const TGetDateSceneArgs& args,
                             const TRunRequest& runRequest,
                             TStorage& storage,
                              const TSource&) const
{
    TGetDateState newScenarioState;
    TGetDateRenderProto renderProto;
    TString tz = runRequest.Client().GetTimezone();

    // Analyze GEO information
    if (!args.GetWhere().Empty()) {
        if (!GetLocationInfo(runRequest, args, renderProto, tz)) {
            // Error receiving geo information
            LOG_INFO(runRequest.Debug().Logger()) << "get_date scenario: unable to receive geo info";
            renderProto.SetPhrase("error_where");
            return TReturnValueRender(&TGetDateScenario::Render, renderProto);
        }
        newScenarioState.MutableWhere()->SetStringValue(args.GetWhere());
        newScenarioState.MutableWhereType()->SetStringValue(args.GetWhereType());
    } else {
        FillCurrentLocationInfo(runRequest, renderProto, tz);
    }

    // Get tense information (present/past/future)
    const TString tenseVerb = args.GetTense();
    if (!tenseVerb.Empty()) {
        newScenarioState.MutableTense()->SetStringValue(tenseVerb);
    }
    TSysDatetimeParser::ETense tense = FillTenseInfo(tenseVerb);
    // Parse final question (may be changed below if Unknown)
    ETargetQuestion question = GetTargetQuestionInfo(args);
    if (question != ETargetQuestion::Default) {
        newScenarioState.MutableQueryTarget()->SetStringValue(args.GetQueryTarget());
    }

    // Save scene arguments to the storage
    *newScenarioState.MutableDate() = args.GetDate();
    storage.SetScenarioState(newScenarioState);


    const NDatetime::TCivilSecond dateCurrent = NDatetime::Convert(TInstant::MilliSeconds(runRequest.Client().GetClientTimeMsGMT().count()), tz);
    TCalendarDatesArray allDates{args};

    if (tenseVerb == "present") {
        // Для слов "сейчас", "теперь" и т.п. добавляем еще один слот даты Today
        allDates.AddToday();
        allDates.CalculateDestinationDates(dateCurrent, tense);
    }

    //
    // allDates содержит полный список дат, которые пришли из запроса
    // Пробуем скомпактить список до разумного количества дат, используя разные эвристики
    //

    // Случай 1. Полный дубликат
    // "'Суббота' 'Суббота' что за день недели".
    // Дубликаты игнорируются
    allDates.CompactVector([](TCalendarDatesArray::TDateInformation& p1, const TCalendarDatesArray::TDateInformation& p2) -> bool {
        return p1.DateSlot.GetRawDatetime() == p2.DateSlot.GetRawDatetime();
    });

    // Случай 2. Разрыв даты
    // "'12 ноября', эээээ, 'в 2021 году' какой день недели"
    // Мердж возможен, если оба слота содержат одинаковый тип данных и не пересекаются по значениям (например, в одном - число, в другом - месяц и год)
    // Результатом будет 1 слот "12 ноября 2021 года"
    allDates.CompactVector([](TCalendarDatesArray::TDateInformation& p1, const TCalendarDatesArray::TDateInformation& p2) -> bool {
        if (p1.DateType == p2.DateType) {
            return p1.DateSlot.Merge(p2.DateSlot);
        }
        return false;
    });

    allDates.CalculateDestinationDates(dateCurrent, tense);

    switch (allDates.Size()) {
        case 0: {
            // Cлучай, когда вопрос задан вообще без адресации к слотам даты (например, "день недели?" или "календарная дата?").
            // В этом случае предполагается, что речь идет о сегодняшнем дне и возвращается TSysDatetimeParser::Today();
            if (question == ETargetQuestion::Unknown) {
                HW_ERROR("get_date scenario: all date slots are unknown, scenario is irrelevant");
            }
            LOG_DEBUG(runRequest.Debug().Logger()) << "No CalendarDate slots";
            TMaybe<TSysDatetimeParser> today = TSysDatetimeParser::Today();
            return Finalize(runRequest.Debug().Logger(), question, tense, *today, dateCurrent, renderProto);
        }
        case 1:
            // Случай большинства простейших запросов, в которых присутствует один единственный слот даты
            // Например "Какая 'сегодня' полная дата?" или "'Суббота' что за число" или "'Первое сентября' какой день недели был"
            if (question == ETargetQuestion::Unknown || question == ETargetQuestion::Default) {
                // Невалидный случай, когда запрос выглядит как "сегодня сегодня сегодня"
                // и не содержит обращения к дню недели/etc
                // Такой запрос считается нерелевантным для ответа
                LOG_DEBUG(runRequest.Debug().Logger()) << "Non relevant for output";
                renderProto.SetPhrase("error");
                return TReturnValueRender(&TGetDateScenario::Render, renderProto).MakeIrrelevantAnswerFromScene();
            } else if (question == ETargetQuestion::DayOfWeek && allDates.At(0).DateType == TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek) {
                // Cтранные вопросы ("пятница какой день недели")
                // Заменим вопрос на дату, чтобы что-то ответить вменяемое
                LOG_DEBUG(runRequest.Debug().Logger()) << "Replace day of week question";
                question = ETargetQuestion::Date;
            } else if (question == ETargetQuestion::Date && allDates.At(0).DateType == TSysDatetimeParser::EDatetimeParser::Fixed) {
                // Cтранные вопросы ("23 февраля какое число")
                // На такие вопросы отвечает поиск, не будем ему мешать
                LOG_DEBUG(runRequest.Debug().Logger()) << "Non relevant for output";
                renderProto.SetPhrase("error");
                return TReturnValueRender(&TGetDateScenario::Render, renderProto).MakeIrrelevantAnswerFromScene();
            }
            // Контроль релативных вопросов и неправильного времени ("вчера какая дата будет")
            if (tense == NAlice::TSysDatetimeParser::ETense::TensePast && allDates.At(0).DateType == TSysDatetimeParser::EDatetimeParser::RelativeFuture) {
                tense = NAlice::TSysDatetimeParser::ETense::TenseFuture;
            } else if (tense == NAlice::TSysDatetimeParser::ETense::TenseFuture && allDates.At(0).DateType == TSysDatetimeParser::EDatetimeParser::RelativePast) {
                tense = NAlice::TSysDatetimeParser::ETense::TensePast;
            }
            LOG_DEBUG(runRequest.Debug().Logger()) << "Single CalendarDate slots with question " << static_cast<int>(question);
            return Finalize(runRequest.Debug().Logger(), question, tense, allDates.At(0).DateSlot, dateCurrent, renderProto);
        case 2:
            // Разрыв даты со смешиванием абсолюта и релатива
            // "Какой день будет '13 октября' 'через пять лет'?"
            // ! Этот случай обработан первым, так как с точки зрения типов слотов нет разницы
            // с "'Сегодня' '15 ноября'" (см следующий кейс). Отыгрываем это эвристикой IsLargeRelative()
            if (allDates.FindWinner({TCalendarDatesArray::EDateType::LargeRelative,
                                     TCalendarDatesArray::EDateType::Fixed})) {
                if (allDates.Merge(0, 1)) {
                    LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (large relative+absolute), merge";
                    return Finalize(runRequest.Debug().Logger(), question, tense, allDates.At(0).DateSlot, dateCurrent, renderProto);
                } else {
                    // Вопрос вида "Какой день будет '13 октября' 'через месяц'?"
                    LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (large relative+absolute), unable to merge";
                    renderProto.SetPhrase("error");
                    return TReturnValueRender(&TGetDateScenario::Render, renderProto).MakeIrrelevantAnswerFromScene();
                }
            }
            break;
        default:
            // Остальные случаи будут рассмотрены ниже
            break;
    }

    Y_ENSURE(allDates.Size() >= 2);

    // Вопросы типа "'Сегодня' 'пятница' [или суббота...]"
    // Выигрывает релятивная дата (сегодня), будет ответ про актуальный день недели (+ да/нет)
    {
        auto winner = allDates.FindWinner({TCalendarDatesArray::EDateType::Relative,
                                        TCalendarDatesArray::EDateType::Weekday});
        if (winner != nullptr) {
            renderProto.SetIsQuestion(allDates.IsSomeDatesMatched(winner) ? "yes" : "no");
            LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (relative+dayofweek[])";
            return Finalize(runRequest.Debug().Logger(), ETargetQuestion::DayOfWeek, tense, winner->DateSlot, dateCurrent, renderProto);
        }
    }

    // Вопрос "'Пятница' - это 'Сегодня' [или вчера...]"
    // TODO: в будущем это будет ответ "Сколько дней до "день недели"
    // Пока что оставляем выигрыш за релативной датой
    {
        auto winner = allDates.FindWinner({TCalendarDatesArray::EDateType::Weekday,
                                       TCalendarDatesArray::EDateType::Relative});
        if (winner != nullptr) {
            renderProto.SetIsQuestion(allDates.IsSomeDatesMatched(winner) ? "yes" : "no");
            LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (dayofweek + relative[])";
            return Finalize(runRequest.Debug().Logger(), ETargetQuestion::Date, tense, winner->DateSlot, dateCurrent, renderProto);
        }
    }

    // Вопросы со смешиванием абсолюта и релатива типа "'Сегодня' '15 ноября' [или 16 ноября]"
    // Выигрывает релятивная дата (сегодня), будет ответ про актуальный день недели
    {
        auto winner = allDates.FindWinner({TCalendarDatesArray::EDateType::Relative,
                                        TCalendarDatesArray::EDateType::Fixed});
        if (winner == nullptr) {
            // Вопросы со смешиванием абсолюта и релатива типа "'15 ноября' это сегодня [или завтра]"
            // TODO: в будущем это будет ответ "Сколько дней до "дата"
            // Пока что оставляем выигрыш за релативной датой
            winner = allDates.FindWinner({TCalendarDatesArray::EDateType::Fixed,
                                        TCalendarDatesArray::EDateType::Relative});
        }
        if (winner != nullptr) {
            renderProto.SetIsQuestion(allDates.IsSomeDatesMatched(winner) ? "yes" : "no");
            LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (relative+absolute[] or absolute + relative[])";
            return Finalize(runRequest.Debug().Logger(), ETargetQuestion::All, tense, winner->DateSlot, dateCurrent, renderProto);
        }
    }

    // Вопросы типа "'Пятница' '20 октября' [21...]"
    // Выигрывает день недели (Пятница), будет ответ про число в этот день недели
    {
        auto winner = allDates.FindWinner({TCalendarDatesArray::EDateType::Weekday,
                                        TCalendarDatesArray::EDateType::Fixed});
        if (winner != nullptr) {
            renderProto.SetIsQuestion(allDates.IsSomeDatesMatched(winner) ? "yes" : "no");
            LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (weekday+absolute[])";
            return Finalize(runRequest.Debug().Logger(), ETargetQuestion::All, tense, winner->DateSlot, dateCurrent, renderProto);
        }
    }

    // Вопросы типа "'20 октября' это 'Пятница'"
    // Выигрывает 20 октября, будет ответ про день недели в это число
    {
        auto winner = allDates.FindWinner({TCalendarDatesArray::EDateType::Fixed,
                                        TCalendarDatesArray::EDateType::Weekday});
        if (winner != nullptr) {
            renderProto.SetIsQuestion(allDates.IsSomeDatesMatched(winner) ? "yes" : "no");
            LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (absolute + wekkday[])";
            return Finalize(runRequest.Debug().Logger(), ETargetQuestion::DayOfWeek, tense, winner->DateSlot, dateCurrent, renderProto);
        }
    }

    // Длинный Релатив + Абсолют + День недели
    // Год назад пятница 13 декабря?
    {
        auto* winner = allDates.FindWinner({TCalendarDatesArray::EDateType::LargeRelative,
                                            TCalendarDatesArray::EDateType::Weekday,
                                            TCalendarDatesArray::EDateType::Fixed});
        if (winner != nullptr) {
            if (allDates.Merge(allDates.FindByType(TCalendarDatesArray::EDateType::LargeRelative),
                               allDates.FindByType(TCalendarDatesArray::EDateType::Fixed))) {
                LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (large relative+absolute), merge";
                return Finalize(runRequest.Debug().Logger(), question, tense, winner->DateSlot, dateCurrent, renderProto);
            } else {
                // Вопрос вида "Какой день будет '13 октября' 'через месяц'?"
                LOG_DEBUG(runRequest.Debug().Logger()) << "Two CalendarDate slots (large relative+absolute), unable to merge";
                renderProto.SetPhrase("error");
                return TReturnValueRender(&TGetDateScenario::Render, renderProto).MakeIrrelevantAnswerFromScene();
            }


            renderProto.SetIsQuestion(allDates.IsAllDatesMatched() ? "yes" : "no");
            LOG_DEBUG(runRequest.Debug().Logger()) << "Three CalendarDate slots (largerelative+absolute+dayofweek)";
            return Finalize(runRequest.Debug().Logger(), ETargetQuestion::All, tense, winner->DateSlot, dateCurrent, renderProto);
        }
    }

    // Релатив + Абсолют + День недели
    // Сегодня пятница 13 декабря?
    {
        auto* winner = allDates.FindWinner({TCalendarDatesArray::EDateType::Relative,
                                            TCalendarDatesArray::EDateType::Weekday,
                                            TCalendarDatesArray::EDateType::Fixed});
        if (winner != nullptr) {
            renderProto.SetIsQuestion(allDates.IsAllDatesMatched() ? "yes" : "no");
            LOG_DEBUG(runRequest.Debug().Logger()) << "Three CalendarDate slots (absolute+dayofweek+relative)";
            return Finalize(runRequest.Debug().Logger(), ETargetQuestion::All, tense, winner->DateSlot, dateCurrent, renderProto);
        }
    }
    LOG_INFO(runRequest.Debug().Logger()) << "get_date scenario: unable to find a winner";
    renderProto.SetPhrase("error");
    return TReturnValueRender(&TGetDateScenario::Render, renderProto).MakeIrrelevantAnswerFromScene();
}

TRetMain TGetDateScene::Finalize(TRTLogger& logger, ETargetQuestion question,
                                 TSysDatetimeParser::ETense tense,
                                 TSysDatetimeParser& dateParsedResult,
                                 const NDatetime::TCivilSecond dateCurrent,
                                 TGetDateRenderProto& renderProto) const {

    // Проверка, что dateParsedResult содержит валидную дату с точки зрения каленаря
    // (компоновки даты в формате "тридцатое ээээ февраля эээээ 2021 года" могут привести к невалидному результату
    if (dateParsedResult.GetParseInfo() == TSysDatetimeParser::EDatetimeParser::Fixed) {
        if (dateParsedResult.GetRawDatetime().Day.IsAbsolute() &&
            dateParsedResult.GetRawDatetime().Month.IsAbsolute() &&
            dateParsedResult.GetRawDatetime().Year.IsAbsolute()) {
            const NDatetime::TYear year = dateParsedResult.GetRawDatetime().Year.Get();
            const NDatetime::TMonth month = dateParsedResult.GetRawDatetime().Month.Get();
            if (dateParsedResult.GetRawDatetime().Day.Get() < 1 || dateParsedResult.GetRawDatetime().Day.Get() > NDatetime::DaysPerMonth(year, month)) {
                renderProto.SetPhrase("wrong_date");
                return TReturnValueRender(&TGetDateScenario::Render, renderProto);
            }
        }
    }

    if (renderProto.GetIsQuestion().Empty()) {
        // Фикс для запросов с днями недели (только если не был вопрос с да/нет)
        // Если в четверг спросить про дату четверга, VINS отвечает про следующий четверг
        // Для аналогичного поведения в SysDateTime надо обязательно указать TenseFuture (при TenseDefault будет возвращен ответ про текущий четверг)
        if (tense == TSysDatetimeParser::TenseDefault && dateParsedResult.GetParseInfo() == TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek) {
            const NDatetime::TCivilDay dayCurrent = NDatetime::TCivilDay{dateCurrent.year(), dateCurrent.month(), dateCurrent.day()};
            const NDatetime::TWeekday weekday = NDatetime::GetWeekday(dayCurrent); // enum class weekday: 0 - Monday, 6 - Sunday
            // Sys Datetime uses day of week - // 0...6 from Sunday
            const int weekdaySysDateTime = weekday == cctz::weekday::sunday ? 0 : static_cast<int>(weekday) + 1;
            if (weekdaySysDateTime == dateParsedResult.GetRawDatetime().DayOfWeek.Get()) {
                tense = TSysDatetimeParser::TenseFuture;
            }
        }
    }

    const auto dateFinal = dateParsedResult.GetTargetDateTime(dateCurrent, tense);
    LOG_INFO(logger) << "Final calculated date: " << dateFinal;

    if (dateParsedResult.GetParseInfo() == TSysDatetimeParser::EDatetimeParser::Fixed) {
        if (dateParsedResult.GetRawDatetime().Day.IsNotFilled()) {
            dateParsedResult.GetRawDatetime().Day.SetAbsolute(dateFinal.day());
        }
        if (dateParsedResult.GetRawDatetime().Month.IsNotFilled()) {
            dateParsedResult.GetRawDatetime().Month.SetAbsolute(dateFinal.month());
        }
        if (dateParsedResult.GetRawDatetime().Year.IsNotFilled() && dateFinal.year() != dateCurrent.year()) {
            // Year will be set only if different with the current
            dateParsedResult.GetRawDatetime().Year.SetAbsolute(dateFinal.year());
        }
    }

    // Remove HH:MM:SS to prevent illegal time-delta answers
    dateParsedResult.GetRawDatetime().Hour.Erase();
    dateParsedResult.GetRawDatetime().Minutes.Erase();
    dateParsedResult.GetRawDatetime().Seconds.Erase();

    // Set final data for rendering
    const NDatetime::TCivilDay dayOnlyFinal = NDatetime::TCivilDay{dateFinal.year(), dateFinal.month(), dateFinal.day()};
    renderProto.SetResultYear(dateFinal.year());
    renderProto.SetResultMonth(dateFinal.month());
    renderProto.SetResultDay(dateFinal.day());
    renderProto.SetResultDayWeek(static_cast<int>(cctz::get_weekday(dayOnlyFinal)));
    if (question == ETargetQuestion::Year || question == ETargetQuestion::AllWithYear) {
        // Для запросов "какой год" всегда ставим отображение года
        renderProto.SetResultYearDifferent(true);
    } else {
        // В остальных случаях год пишется только если он не совпадает с текущим
        renderProto.SetResultYearDifferent(dateCurrent.year() != dateFinal.year() ? true : false);
    }
    renderProto.SetResultWeekNmb(NDatetime::GetYearWeek(dayOnlyFinal));

    if (dateFinal > dateCurrent) {
        renderProto.SetTense("future");
    } else if (dateFinal < dateCurrent) {
        renderProto.SetTense("past");
    }
    renderProto.SetSourceDate(JsonToString(dateParsedResult.GetAsJsonDatetime()));
    switch (dateParsedResult.GetParseInfo()) {
        case TSysDatetimeParser::EDatetimeParser::Unknown:
        case TSysDatetimeParser::EDatetimeParser::Mix:
            renderProto.SetSourceType("");
            break;
        case TSysDatetimeParser::EDatetimeParser::Fixed:
            renderProto.SetSourceType("fixed");
            break;
        case TSysDatetimeParser::EDatetimeParser::RelativeFuture:
        case TSysDatetimeParser::EDatetimeParser::RelativePast:
        case TSysDatetimeParser::EDatetimeParser::RelativeMix:
            renderProto.SetSourceType("relative");
            break;
        case TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek:
            renderProto.SetSourceType("dayofweek");
            break;
    }
    //
    // Get target question from granet
    // date, day of week or both
    //
    switch (question) {
        case ETargetQuestion::Date:
        case ETargetQuestion::Year:
            renderProto.SetPhrase("day_month_year");
            return TReturnValueRender(&TGetDateScenario::Render, renderProto);
        case ETargetQuestion::DayOfWeek:
            renderProto.SetPhrase("day_of_week");
            return TReturnValueRender(&TGetDateScenario::Render, renderProto);
        case ETargetQuestion::All:
        case ETargetQuestion::AllWithYear:
        case ETargetQuestion::Default:
            renderProto.SetPhrase("day_all");
            return TReturnValueRender(&TGetDateScenario::Render, renderProto);
        case ETargetQuestion::Week:
            renderProto.SetPhrase("what_week");
            return TReturnValueRender(&TGetDateScenario::Render, renderProto);
        case ETargetQuestion::Month:
            renderProto.SetPhrase("what_month");
            return TReturnValueRender(&TGetDateScenario::Render, renderProto);
        case ETargetQuestion::Unknown:
        default:
            break;
    }
    HW_ERROR("Undefined question");
}

}  // namespace NAlice::NHollywood::NGetDate
