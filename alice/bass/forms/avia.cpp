#include "avia.h"
#include "directives.h"
#include "geodb.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/request.h>

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <kernel/geodb/countries.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/neh/http_common.h>

#include <util/generic/cast.h>
#include <util/generic/yexception.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS {
namespace {
TSlot* GetFromSlot(TContext& r) {
    return r.GetOrCreateSlot(TStringBuf("geo_from"), TStringBuf("string"));
}

TSlot* GetToSlot(TContext& r) {
    return r.GetOrCreateSlot(TStringBuf("geo_to"), TStringBuf("string"));
}

TSlot* GetErrorSlot(TContext& r) {
    return r.GetOrCreateSlot(TStringBuf("error"), TStringBuf("string"));
}

TSlot* GetErrorCodeSlot(TContext& r) {
    return r.GetOrCreateSlot(TStringBuf("error_code"), TStringBuf("string"));
}

TSlot* GetMinPriceSlot(TContext& r) {
    return r.GetOrCreateSlot(TStringBuf("min_price"), TStringBuf("number"));
}

TSlot* GetMinPriceDateSlot(TContext& r) {
    return r.GetOrCreateSlot(TStringBuf("min_price_date"), TStringBuf("datetime"));
}

TString GetUtmSource(const TContext& ctx) {
    const auto& clientInfo = ctx.MetaClientInfo();
    if (clientInfo.IsYaStroka()) {
        return "alice_stroka";
    }

    if (clientInfo.IsYaBrowser()) {
        return "alice_yabro";
    }

    if (clientInfo.IsSearchApp()) {
        return "alice_pp";
    }

    if (clientInfo.IsSmartSpeaker()) {
        return "alice_station";
    }

    return "alice";
}

bool IsMonthQuery(const NSc::TValue& datetime) {
    return datetime["days"].IsNull() &&
           !datetime["days_relative"].GetBool(false) &&
           (!datetime["months"].IsNull() || datetime["months_relative"].GetBool(false)) &&
           !datetime["week_relative"].GetBool(false) &&
           !datetime["seconds_relative"].GetBool(false) &&
           !datetime["hours_relative"].GetBool(false) &&
           !datetime["minutes_relavtie"].GetBool(false);
}

const i32 DAYS_IN_MONTH[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

TDateTime::TSplitTime GetDate(const TDateTime::TSplitTime& datetime) {
    return TDateTime::TSplitTime(datetime.TimeZone(),
                                 IntegerCast<ui32, int>(datetime.RealYear()),
                                 IntegerCast<ui32, int>(datetime.RealMonth()),
                                 IntegerCast<ui32, int>(datetime.MDay()));
}

TDateTime::TSplitTime UserTimeFromContext(TContext& ctx) {
    return TDateTime::TSplitTime(NDatetime::GetTimeZone(ctx.UserTimeZone()), ctx.Meta().Epoch());
}


i8 NextMonth(i8 month) {
    return (month == 11) ? 12 : ((month + 1) % 12);
}


class WaitResponse : public IObjectInQueue {
public:
    WaitResponse(NHttpFetcher::THandle::TRef handle)
        : Handle(handle)
    {
    }

    void Process(void*) override {
        auto response = Handle->Wait();
        if (!response) {
            LOG(ERR) << "Timeout" << Endl;
        } else if (response->IsError()) {
            LOG(ERR) << "Error: " << response->GetErrorText() << Endl;
        } else {
            LOG(DEBUG) << "Response: " << response->Data << Endl;
        }
    }

private:
    NHttpFetcher::THandle::TRef Handle;
};


class TPointResolveException : public yexception {

};

const TStringBuf NO_AIRPORT_ERROR = "no_airport";
}

const TStringBuf AVIA_MAIN_FORM = "personal_assistant.scenarios.avia";
const TStringBuf AVIA_CHECKOUT_FORM = "personal_assistant.scenarios.avia__checkout";

class TAviaPriceInfo {
public:
    TAviaPriceInfo(ui64 fromId, ui64 toId, TDateTime::TSplitTime& dateForward, TMaybe<TDateTime::TSplitTime>& dateBackward, i32 value,
                   const TString& currency)
            : FromId(fromId)
            , ToId(toId)
            , Value(value)
            , Currency(currency)
            , DateForward(dateForward)
            , DateBackward(dateBackward)
    {
    }

    TAviaPriceInfo(const TAviaPriceInfo& other) = default;
    TAviaPriceInfo& operator=(const TAviaPriceInfo& other) = default;

    TAviaPriceInfo()
        : FromId(0)
        , ToId(0)
        , Value(0)
        , Currency("")
    {
    }

    i32 GetValue() const {
        return Value;
    }

    const TDateTime::TSplitTime& GetDateForward() const {
        return DateForward;
    }

    const TMaybe<TDateTime::TSplitTime>& GetDateBackward() const {
        return DateBackward;
    }

    const TString& GetCurrency() const {
        return Currency;
    }

    ui64 GetFromId() const {
        return FromId;
    }

    ui64 GetToId() const {
        return ToId;
    }

private:
    ui64 FromId;
    ui64 ToId;
    i32 Value;
    TString Currency;
    TDateTime::TSplitTime DateForward = TDateTime::TSplitTime(NDatetime::GetUtcTimeZone(), 0);
    TMaybe<TDateTime::TSplitTime> DateBackward = TDateTime::TSplitTime(NDatetime::GetUtcTimeZone(), 0);
};


class TAviaTDApiClient {
public:
    TAviaTDApiClient(const TContext& ctx)
        : Context(ctx)
    {
    }

    NHttpFetcher::THandle::TRef Init(const TAviaForm& form) const {
        auto request = Context.GetSources().AviaTDApiInit().Request();
        request->AddCgiParams(GetCgi(form));
        request->SetMethod(TStringBuf("GET"));
        return request->Fetch();
    }

private:
    const TContext& Context;
    static constexpr TStringBuf SERVICE_NAME = "alice";
    TCgiParameters GetCgi(const TAviaForm& form) const {
        TCgiParameters cgi = CommonCgi();
        cgi.InsertUnescaped(TStringBuf("point_from"), form.From.GetRef().CreateKey());
        cgi.InsertUnescaped(TStringBuf("point_to"), form.To.GetRef().CreateKey());

        if (const TDateTime::TSplitTime* dateForwardPtr = form.DateForward.Get()) {
            cgi.InsertUnescaped(TStringBuf("date_forward"), dateForwardPtr->ToString("%Y-%m-%d"));
        } else {
            // If DateForward is empty, send request for tomorrow
            // We do not care about precise calculation of tomorrow
            // Use client time for tests and server time otherwise
            ui64 currTime = Context.MetaClientInfo().IsSearchAppTest() ? Context.Meta().Epoch() : Context.Now().Seconds();
            TInstant tomorrow = TInstant::Seconds(currTime) + TDuration::Seconds(60 * 60 * 24);
            cgi.InsertUnescaped(TStringBuf("date_forward"), tomorrow.FormatLocalTime("%Y-%m-%d"));
        }
        if (const TDateTime::TSplitTime* dateBackwardPtr = form.DateForward.Get()) {
            cgi.InsertUnescaped(TStringBuf("date_backward"), dateBackwardPtr->ToString("%Y-%m-%d"));
        }

        cgi.InsertUnescaped(TStringBuf("adults"), ToString(form.Adults));
        cgi.InsertUnescaped(TStringBuf("children"), ToString(form.Children));
        cgi.InsertUnescaped(TStringBuf("infants"), ToString(form.Infants));

        return cgi;
    }

    TCgiParameters CommonCgi() const {
        TCgiParameters cgi;
        cgi.InsertUnescaped(TStringBuf("service"), SERVICE_NAME);
        cgi.InsertUnescaped(TStringBuf("lang"), TStringBuf("ru"));
        cgi.InsertUnescaped(TStringBuf("klass"), TStringBuf("economy"));
        return cgi;
    }
};


class TAviaPriceFetcher {
public:
    TAviaPriceFetcher(TContext& ctx)
        : Ctx(ctx)
        , UserTime(UserTimeFromContext(ctx))
    {
    }

    void FetchMinPrices(const TVector<ui64>& fromIds, const TVector<ui64>& toIds, const TMaybe<TDateTime::TSplitTime>& dateForward,
                        const TMaybe<TDateTime::TSplitTime>& dateBackward, i32 windowSize, TVector<TAviaPriceInfo>& out) {

        Y_ASSERT(fromIds.size() != 0);
        Y_ASSERT(toIds.size() != 0);
        TString requestBody = GetRequest(fromIds, toIds, dateForward, dateBackward, windowSize);
        LOG(DEBUG) << "MinPrice request body: " << requestBody << Endl;

        auto request = Ctx.GetSources().AviaPriceIndex().Request();
        request->SetMethod("POST");
        request->SetBody(requestBody);
        request->SetContentType("application/json");

        auto handler = request->Fetch()->Wait();
        if (handler->IsError()) {
            LOG(ERR) << "MinPrice HTTP Error" << handler->GetErrorText() << Endl;
            return;
        }

        LOG(DEBUG) << "Price-index response: " << handler->Data << Endl;
        FillPricesFromResponse(handler->Data, out);
    }

    void FetchMonthMinPrices(ui64 fromId, ui64 toId, i8 monthNumber, TVector<TAviaPriceInfo>& out) const {
        Y_ASSERT(fromId != 0);
        Y_ASSERT(toId != 0);
        Y_ASSERT((1 <= monthNumber) && (monthNumber <= 12));

        TString requestBody = GetRequestForMonth(fromId, toId, monthNumber);
        auto request = Ctx.GetSources().AviaPriceIndexMinPrice().Request();
        request->SetMethod("POST");
        request->SetBody(requestBody);
        request->SetContentType("application/json");

        auto handler = request->Fetch()->Wait();
        if (handler->IsError()) {
            LOG(ERR) << "MinPrice HTTP Error" << handler->GetErrorText() << Endl;
            return;
        }

        LOG(DEBUG) << "Price-index response: " << handler->Data << Endl;
        FillPricesFromResponse(handler->Data, out);
    }

private:
    TContext& Ctx;
    const TDateTime UserTime;

    TString GetRequestForMonth(ui64 fromId, ui64 toId, i8 monthNumber) const {
        int userMonth = UserTime.SplitTime().RealMonth();
        i32 startDay = (monthNumber == userMonth) ? UserTime.SplitTime().MDay() : 1;

        i32 year = UserTime.SplitTime().RealYear();
        if (monthNumber < userMonth) {
            year += 1;
        }

        i32 endDay = GetDaysInMonth(monthNumber, year);

        NSc::TValue minRequests;
        for (i32 currentDay = startDay; currentDay <= endDay; ++currentDay) {
            NSc::TValue minRequest;
            minRequest["from_id"] = fromId;
            minRequest["to_id"] = toId;
            minRequest["forward_date"] = TStringBuilder() << ToString(year) << "-" << ToString(monthNumber) << "-" << ToString(currentDay);
            minRequest["backward_date"] = NSc::Null();
            minRequests.Push(minRequest);
        }

        NSc::TValue request;
        request["min_requests"] = minRequests;

        return request.ToJson();
    }

    static i32 GetDaysInMonth(i8 monthNumber, i32 year) {
        if ((monthNumber == 2) && IsLeapYear(year)) {
            return 29;
        }

        return DAYS_IN_MONTH[monthNumber];
    }

    static bool IsLeapYear(i32 year) {
        if (year % 100 != 0) {
            return year % 4 == 0;
        }

        return year % 400 == 0;
    }

    TString GetRequest(const TVector<ui64>& fromIds, const TVector<ui64>& toIds,
                       TMaybe<TDateTime::TSplitTime> dateForward,
                       TMaybe<TDateTime::TSplitTime> dateBackward, i32 windowSize) {
        NSc::TValue directions;
        for (const auto& fromId: fromIds) {
            for (const auto& toId: toIds) {
                NSc::TValue direction;
                direction["from_id"] = fromId;
                direction["to_id"] = toId;
                directions.Push(direction);
            }
        }

        NSc::TValue request;
        request["directions"] = directions;
        request["forward_date"] = dateForward.Empty() ? UserTime.SplitTime().ToString("%Y-%m-%d") : dateForward.Get()->ToString("%Y-%m-%d");
        request["window_size"] = windowSize;

        if (dateBackward.Empty()) {
            request["backward_date"] = NSc::Null();
        } else {
            request["backward_date"] = dateBackward.Get()->ToString("%Y-%m-%d");
        }
        request["results_per_direction"] = 1;

        return request.ToJson();
    }

    TMaybe<TDateTime::TSplitTime> ParseDate(TStringBuf str) const {
        ui32 year;
        ui32 month;
        ui32 day;
        bool success = TryFromString(str.substr(0, 4), year) &&
                       TryFromString(str.substr(5, 2), month) &&
                       TryFromString(str.substr(8, 2), day);

        if (success) {
            return TDateTime::TSplitTime(UserTime.SplitTime().TimeZone(), year, month, day);
        }

        return TMaybe<TDateTime::TSplitTime>();
    }

    void FillPricesFromResponse(const TStringBuf response, TVector<TAviaPriceInfo>& out) const {
        const NSc::TValue parsedResponse = NSc::TValue::FromJson(response);
        if (parsedResponse["status"].GetString() != "ok") {
            LOG(ERR) << "Price Index request request failed." << Endl;
            return;
        }

        const NSc::TArray& array = parsedResponse["data"].GetArray();

        out.clear();
        for (const auto &item: array) {
            ui64 fromId = item["from_id"].GetIntNumber();
            ui64 toId = item["to_id"].GetIntNumber();
            TMaybe<TDateTime::TSplitTime> minPriceDateForward = ParseDate(item["forward_date"].GetString());
            if (minPriceDateForward.Empty()) {
                LOG(ERR) << "Bad date in price index response: " << item["forward_date"] << Endl;
                continue;
            }

            NSc::TValue dateBackwardResp = item["backward_date"];
            TMaybe<TDateTime::TSplitTime> minPriceDateBackward = dateBackwardResp.IsNull()
                                                                 ? TMaybe<TDateTime::TSplitTime>()
                                                                 : ParseDate(dateBackwardResp.GetString());
            const NSc::TValue& minPrice = item["min_price"];
            i32 value = minPrice["value"].GetIntNumber();

            TStringBuf currency = minPrice["currency"].GetString();
            out.push_back(TAviaPriceInfo{fromId, toId, minPriceDateForward.GetRef(), minPriceDateBackward, value,
                                         TString{currency}});
        }
    }

};


class TAviaResolver {
public:
    TAviaResolver(TContext& ctx)
        : Ctx(ctx)
    {
    }

    TMaybe<TAviaPoint> GetPointByTitle(const TStringBuf title) const {
        TCgiParameters cgi;
        cgi.InsertUnescaped("query", title);
        cgi.InsertUnescaped("national_version", "ru");
        cgi.InsertUnescaped("includePrices", "true");
        cgi.InsertUnescaped("lang", "ru");
        cgi.InsertUnescaped("field", "to");
        auto response = Ctx.GetSources().AviaSuggests().Request()->AddCgiParams(cgi).Fetch()->Wait();
        if (response->IsError()) {
            LOG(ERR) << "Avia suggests error: " << response->GetErrorText() << Endl;
            return TMaybe<TAviaPoint>{};
        }

        NSc::TValue parsedResponse = NSc::TValue::FromJson(response->Data);
        LOG(DEBUG) << "Suggest response: " << response->Data << Endl;

        const NSc::TArray& variants = parsedResponse[1].GetArray();
        for (const NSc::TValue& variant: variants) {
            TMaybe<TAviaPoint> out = GetPoint(variant);
            if (!out.Empty()) {
                return out;
            }
        }
        return TMaybe<TAviaPoint>{};
    }

    TAviaForm FillFormFromCtx() {
        TAviaForm form;
        FillPoints(form);

        const TDateTime::TSplitTime userTime = UserTimeFromContext(Ctx);
        TSlot* dateForwardSlot = Ctx.GetSlot("date_forward");
        if (!IsSlotEmpty(dateForwardSlot)) {
            LOG(DEBUG) << "Start forward date analyzing" << Endl;
            if (IsMonthQuery(dateForwardSlot->Value)) {
                LOG(DEBUG) << "Month query" << Endl;
                const auto& monthsRelative = dateForwardSlot->Value["months_relative"];
                form.MonthNumber = dateForwardSlot->Value["months"].GetIntNumber(0);
                if (!monthsRelative.IsNull() && monthsRelative.GetBool(false)) {
                    form.MonthNumber += userTime.RealMonth();
                }

            } else {
                form.DateForward = DateTimeFromSlot(dateForwardSlot, TDateTime(userTime));
                form.MonthNumber = 0;
            }
        }

        TSlot* dateBackwardSlot = Ctx.GetSlot("date_backward");
        if (!IsSlotEmpty(dateBackwardSlot) && !IsMonthQuery(dateBackwardSlot->Value)) {
            form.DateBackward = DateTimeFromSlot(dateBackwardSlot, TDateTime(userTime));
        }

        if (!form.DateForward.Empty() && !form.DateBackward.Empty() && form.DateForward.Get()->AsTimeT() > form.DateBackward.Get()->AsTimeT()) {
            std::swap(form.DateForward, form.DateBackward);
        }

        return form;
    }

    void AddSuggests(TStringBuf field, const NSc::TValue& otherSlot, const TMaybe<TAviaPoint>& otherPoint, ui64 suggestCount = 3) {
        TCgiParameters cgi = GenerateCgi(NSc::TValue(), otherSlot, field, otherPoint);
        NHttpFetcher::TRequestPtr request = Ctx.GetSources().AviaSuggests().Request();
        auto response = Ctx.GetSources().AviaSuggests().Request()->AddCgiParams(cgi).Fetch()->Wait();
        if (response->IsError()) {
            LOG(ERR) << "Avia suggests error: " << response->GetErrorText() << Endl;
            return;
        }

        NSc::TValue parsedResponse = NSc::TValue::FromJson(response->Data);
        if (parsedResponse.IsNull()) {
            LOG(ERR) << "Can not parse avia suggests response. Response:" << response->Data << Endl;
            return;
        };

        const NSc::TArray& variants = parsedResponse[1].GetArray();
        for (size_t i = 0; i < Min(suggestCount, variants.size()); ++i) {
            AddSuggestVariant(field, variants[i]);
        }
    }

    void FillCountryCities(const TAviaForm& form, TVector<TAviaPoint>& out, ui64 suggestCount=5) {
        LOG(DEBUG) << "Add cities for country " << form.To.Get()->GetTitle() << Endl;
        TCgiParameters cgi;
        cgi.InsertUnescaped("query", form.To.Get()->GetTitle());
        cgi.InsertUnescaped("other_point", form.From.Get()->CreateKey());
        cgi.InsertUnescaped("national_version", "ru");
        cgi.InsertUnescaped("includePrices", "true");
        cgi.InsertUnescaped("lang", "ru");
        cgi.InsertUnescaped("field", "to");

        NHttpFetcher::TRequestPtr request = Ctx.GetSources().AviaSuggests().Request();
        auto response = Ctx.GetSources().AviaSuggests().Request()->AddCgiParams(cgi).Fetch()->Wait();
        if (response->IsError()) {
            LOG(ERR) << "Avia suggests error: " << response->GetErrorText() << Endl;
            return;
        }

        NSc::TValue parsedResponse = NSc::TValue::FromJson(response->Data);
        if (parsedResponse.IsNull()) {
            LOG(ERR) << "Can not parse avia suggests response. Response:" << response->Data << Endl;
            return;
        };

        const NSc::TArray& variants = parsedResponse[1].GetArray();
        if (variants.size() == 0) {
            LOG(DEBUG) << "Empty suggest respone" << Endl;
            return;
        }

        for (const NSc::TValue& variant: variants) {
            TMaybe<TAviaPoint> suggestPoint = GetPoint(variant);
            if (suggestPoint.Empty() || !suggestPoint.Get()->IsCountry()) {
                continue;
            }

            const NSc::TArray& firstVariant = variant.GetArray();
            if (firstVariant.size() < 4) {
                LOG(DEBUG) << "Suggest response does not contain cities filed" << Endl;
                continue;
            }

            const NSc::TArray& cities = firstVariant[3];
            for (size_t i = 0; i < Min(suggestCount, cities.size()); ++i) {
                TMaybe<TAviaPoint> point = GetPoint(cities[i]);
                if (!point.Empty()) {
                    out.push_back(point.GetRef());
                }
            }
            break;
        }
    }

    void AddSuggestByTitle(TStringBuf field, const TStringBuf title) {
        NSc::TValue json;
        json["caption"].SetString(title);

        TContext::TSlot slot((field == "to") ? "geo_to" : "geo_from", "string");
        slot.Value.SetString(title);

        NSc::TValue formUpdate;
        formUpdate["name"] = Ctx.FormName();
        formUpdate["slots"].SetArray().Push(slot.ToJson(nullptr));
        formUpdate["resubmit"].SetBool(true);

        Ctx.AddSuggest(field == "to" ? "avia__point_to" : "avia__point_from", std::move(json), std::move(formUpdate));
    }

    void AddDefaultCountrySuggest(TStringBuf field) {
        static const TStringBuf RUSSIA = "Россия";
        static const TStringBuf FRANCE = "Франция";
        static const TStringBuf TURKEY = "Турция";

        NGeobase::TId userCountry = NGeobase::UNKNOWN_REGION;
        if (NAlice::IsValidId(Ctx.UserRegion())) {
            userCountry = Ctx.GlobalCtx().GeobaseLookup().GetCountryId(Ctx.UserRegion());
        }
        if (userCountry == NGeoDB::TURKEY_ID) {
            AddSuggestByTitle(field, RUSSIA);
            AddSuggestByTitle(field, FRANCE);
        } else if (userCountry == NGeoDB::FRANCE_ID) {
            AddSuggestByTitle(field, RUSSIA);
            AddSuggestByTitle(field, TURKEY);
        } else {
            AddSuggestByTitle(field, TURKEY);
            AddSuggestByTitle(field, FRANCE);
        }
    }

    void FindNearestsCities(ui64 cityAviaId, ui64 maxDistance, bool onlyWithAirports, int count, TVector<TString>& out) {
        NHttpFetcher::TRequestPtr request = Ctx.GetSources().AviaBackend().Request();
        request->AddCgiParam(TStringBuf("name"), TStringBuf("nearestSettlements"));
        request->SetContentType("application/json");
        request->SetMethod("POST");

        request->SetBody(GetNearestCitiesRequestBody(cityAviaId, maxDistance, onlyWithAirports, count).ToJson());
        NHttpFetcher::TResponse::TRef result = request->Fetch()->Wait();

        if (result->IsError()) {
            LOG(ERR) << "AviaBackend nearestSettlement HTTP Error: " << result->GetErrorText() << Endl;
            return;
        }
        ParseNearestCitiesResponse(result->Data, out);

    }

private:
    void ParseNearestCitiesResponse(const TString& response, TVector<TString>& out) {
        LOG(DEBUG) << "Backend response: " << response << Endl;
        NSc::TValue parsed = NSc::TValue::FromJson(response);

        const TStringBuf& status = parsed["status"].GetString();
        if (status != TStringBuf("success")) {
            LOG(ERR) << "AviaBackend Error:" << parsed["reason"].ToJson() << Endl;
            return ;
        }

        const NSc::TValue& data = parsed["data"][0].SetArray();
        for (size_t i = 0; i != data.ArraySize(); ++i) {
            out.push_back(TString(data[i]["title"].GetString()));
        }

    }
    NSc::TValue GetNearestCitiesRequestBody(ui64 cityAviaId, ui64 maxDistance, bool onlyWithAirports, int count) {
        NSc::TValue params;
        params["pointId"] = cityAviaId;
        params["lang"] = "ru";
        params["onlyWithAirports"] = NSc::TValue().SetBool(onlyWithAirports);
        params["count"] = count;
        params["maxDistance"] = maxDistance;

        NSc::TValue request;
        request["name"] = "nearestSettlements";
        request["params"] = params;
        NSc::TValue fields;
        fields.SetArray().Push("title");
        request["fields"] = fields;

        NSc::TValue body;
        body.SetArray().Push(request);
        return body;
    }
    void AddSuggestVariant(TStringBuf field, const NSc::TArray& variant) {
        if (variant.size() < 2) {
            LOG(ERR) << "Bad suggest variant" << Endl;
            return;
        }
        const auto title = variant[1].GetString();
        AddSuggestByTitle(field, title);
    }

    void FillPoints(TAviaForm& form) {
        TSlot* fromSlot = Ctx.GetSlot("geo_from", "string");
        TSlot* toSlot = Ctx.GetSlot("geo_to", "string");
        ResolvePoint(fromSlot, nullptr, "from", form.From, TMaybe<TAviaPoint>());
        if (!IsSlotEmpty(toSlot)) {
            ResolvePoint(toSlot, nullptr, "to", form.To, TMaybe<TAviaPoint>());
        }

        ResolveCityInCountry(form.From, form.To);

    }

    void ResolveCityInCountry(TMaybe<TAviaPoint>& from, TMaybe<TAviaPoint>& to) {
        if (from.Empty() || to.Empty()) {
            return;
        }

        TAviaPoint& fromPoint = from.GetRef();
        TAviaPoint& toPoint = to.GetRef();
        int countriesInQuery = 0;
        if (fromPoint.IsCountry()) {
            ++countriesInQuery;
        }

        if (toPoint.IsCountry()) {
            ++countriesInQuery;
        }
        if (countriesInQuery != 1) {
            return;
        }

        const TString& countryTitle = fromPoint.IsCountry() ? fromPoint.GetTitle() : toPoint.GetTitle();
        const TString& anotherCountry = fromPoint.IsCountry() ? toPoint.GetCountryTitle() : fromPoint.GetCountryTitle();

        if (countryTitle == anotherCountry) {
            // Country and city in the same country
            // Left only city and substitute user geo position
            if (toPoint.IsCountry()) {
                to = from;
            }

            ResolvePoint(nullptr, nullptr, TStringBuf("from"), from, TMaybe<TAviaPoint>{});
        }
    }

    TMaybe<TDateTime::TSplitTime> DateTimeFromSlot(TSlot* slot, const TDateTime& userTime) {
        if (IsSlotEmpty(slot)) {
            return {};
        }

        std::unique_ptr<TDateTimeList> tdl;
        try {
            tdl = TDateTimeList::CreateFromSlot<TContext::TSlot>(slot, nullptr, userTime, {1, true});
        } catch (...) {
            return {};
        }

        if (tdl->IsNow()) {
            return userTime.SplitTime();
        }

        TDateTime::TSplitTime result = tdl->cbegin()->SplitTime();
        time_t userDay = GetDate(userTime.SplitTime()).AsTimeT();
        time_t resultTime = result.AsTimeT();
        if (resultTime >= userDay) {
            return result;
        }

        if (slot->Value["months"].GetIntNumber(0) == 0) {
            // Month is not specified in user query
            result.Add(TDateTime::TSplitTime::EField::F_MON, 1);
        } else {
            // Month and day are specified
            result.Add(TDateTime::TSplitTime::EField::F_YEAR, 1);
        }

        return result;
    }

    TCgiParameters GenerateCgi(const NSc::TValue& slotValue, const NSc::TValue& otherSlotValue, TStringBuf field,
                               const TMaybe<TAviaPoint>& otherPoint) const {
        TCgiParameters cgi;
        NGeobase::TId cityGeoId = NGeobase::UNKNOWN_REGION;
        const auto& geobase = Ctx.GlobalCtx().GeobaseLookup();
        if (Ctx.Meta().HasLocation()) {
            cityGeoId = NAlice::ReduceGeoIdToCity(geobase, Ctx.UserRegion());
            cgi.InsertUnescaped("client_city", TStringBuilder() << cityGeoId);
        }
        cgi.InsertUnescaped("national_version", "ru");
        cgi.InsertUnescaped("lang", "ru");

        TString value = TString{slotValue.GetString()};
        if (slotValue.IsNull() && field == "from") {
            // Use user location as field value
            NAlice::GeoIdToNames(geobase, cityGeoId, "ru", &value, nullptr);
        }
        cgi.InsertUnescaped("query", value);

        cgi.InsertUnescaped("includePrices", "true");
        cgi.InsertUnescaped("onlyWithAirports", "false");
        cgi.InsertUnescaped("need_country", "true");
        if (!otherSlotValue.IsNull()) {
            cgi.InsertUnescaped("other_query", otherSlotValue.GetString());
        }
        cgi.InsertUnescaped("field", field);

        if (!otherPoint.Empty()) {
            cgi.InsertUnescaped("other_point", otherPoint.Get()->CreateKey());
        }

        return cgi;
    }

    NSc::TValue GetSlotValueSafe(const TSlot* slot) {
        return (slot == nullptr) ? NSc::Null() : slot->Value;
    }

    void ResolvePoint(TSlot* slot, TSlot* otherSlot, TStringBuf field, TMaybe<TAviaPoint>& out, const TMaybe<TAviaPoint>& otherPoint) {
        LOG(DEBUG) << "Start point resolving" << Endl;

        TCgiParameters cgi = GenerateCgi(
            GetSlotValueSafe(slot),
            GetSlotValueSafe(otherSlot),
            field,
            otherPoint);

        auto response = Ctx.GetSources().AviaSuggests().Request()->AddCgiParams(cgi).Fetch()->Wait();
        if (response->IsError()) {
            LOG(ERR) << "Avia suggests error: " << response->GetErrorText() << Endl;
            ythrow TPointResolveException() << response->GetErrorText();
        }

        NSc::TValue parsedResponse = NSc::TValue::FromJson(response->Data);
        LOG(DEBUG) << "Suggest response: " << response->Data << Endl;

        const NSc::TArray& variants = parsedResponse[1].GetArray();
        if (variants.size() == 0) {
            return;
        }

        // First, try to find point with code (IATA or Sirena)
        out = FindWithCode(variants);
        if (!out.Empty()) {
            return;
        }

        out = FindNotHidden(variants);
    }

    TMaybe<TAviaPoint> FindWithCode(const NSc::TArray& variants) const {
        for (const NSc::TValue& variant: variants) {
            const NSc::TArray& array = variant.GetArray();
            if (!CheckSuggestVariant(array)) {
                LOG(ERR) << "Bad suggest format" << Endl;
                continue;
            }

            const NSc::TValue& item = array[2];
            TStringBuf pointCode = item["point_code"].GetString();
            if (pointCode) {
                return GetPoint(array);
            }
        }

        return {};
    }

    TMaybe<TAviaPoint> FindNotHidden(const NSc::TArray& variants) const {
        for (const NSc::TValue& variant: variants) {
            const NSc::TArray& array = variant.GetArray();
            if (!CheckSuggestVariant(array)) {
                LOG(ERR) << "Bad suggest format" << Endl;
                continue;
            }

            const NSc::TValue& item = array[2];
            if (item["hidden"].GetIntNumber() != 1) {
                return GetPoint(array);
            }
        }

        return {};
    }

    bool CheckSuggestVariant(const NSc::TArray& variantAsArray) const {
        return variantAsArray.size() >= 3;
    }

    TMaybe<TAviaPoint> GetPoint(const NSc::TArray& array) const {
        if (!CheckSuggestVariant(array)) {
            LOG(ERR) << "Bad suggest format" << Endl;
            return {};
        }

        const NSc::TValue& item = array[2];
        TStringBuf codeValue = item["point_key"].GetString();

        TAviaPoint::EPointType pointType = ConvertSuggestType(array[0].GetIntNumber());
        if (pointType == TAviaPoint::EPointType::Station) {
            LOG(DEBUG) << "Change airport " << codeValue << " to city" << Endl;
            TStringBuf cityTitle = item["city_title"].GetString();
            if (!cityTitle.empty()) {
                return GetPointByTitle(item["city_title"].GetString());
            }
        }

        bool hasAirport = item["have_not_hidden_airport"].GetIntNumber(0) != 0;
        ui64 aviaId = 0;
        if (!TryFromString(codeValue.substr(1), aviaId)) {
            LOG(ERR) << "Bad point key: " << codeValue << Endl;
            return TMaybe<TAviaPoint>();
        };

        LOG(DEBUG) << "Add point: " << codeValue << Endl;
        return TAviaPoint{
            aviaId,
            ConvertSuggestType(array[0].GetIntNumber()),
            array[1].GetString(),
            item["point_code"].GetString(),
            hasAirport,
            item["country_title"].GetString()
        };

    }

    TAviaPoint::EPointType ConvertSuggestType(ui64 suggestType) const {
        if (suggestType == 0) {
            return TAviaPoint::EPointType::Settlement;
        }

        if (suggestType == 1) {
            return TAviaPoint::EPointType::Station;
        }

        return TAviaPoint::EPointType::Country;
    }

private:
    TContext& Ctx;
};


TAviaFormHandler::TAviaFormHandler(IThreadPool& threadPool)
    : ThreadPool(threadPool)
{

}

void TAviaFormHandler::Register(THandlersMap* handlers, IGlobalContext& globalCtx) {
    auto handlerFabric = [&globalCtx]() {
        return MakeHolder<TAviaFormHandler>(globalCtx.AviaThreadPool());
    };
    handlers->emplace(AVIA_MAIN_FORM, handlerFabric);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.avia__ellipsis"), handlerFabric);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.avia__ask_to__ellipsis"), handlerFabric);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.avia__ask_to"), handlerFabric);
    handlers->emplace(AVIA_CHECKOUT_FORM, handlerFabric);
}

TResultValue TAviaFormHandler::Do(TRequestHandler& r) {
    ResetFields(r);
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::AVIA);
    try {
        TAviaForm queryForm = TAviaResolver(r.Ctx()).FillFormFromCtx();
        return GenerateResult(r, queryForm);
    } catch (TPointResolveException& e) {
        LOG(ERR) << e.what() << Endl;
        return AddErrorMessage(r.Ctx());
    }
}


TResultValue TAviaFormHandler::AddErrorMessage(TContext& ctx) const {
    GetErrorCodeSlot(ctx)->Value = "internal_error";
    ctx.AddTextCardBlock("without_cards");
    return TResultValue();
}

TResultValue TAviaFormHandler::GenerateResult(NBASS::TRequestHandler& r, const NBASS::TAviaForm& form) const {
    TContext& ctx = r.Ctx();
    TSlot* errorCodeSlot = GetErrorCodeSlot(ctx);
    TSlot* errorSlot = GetErrorSlot(ctx);
    TSlot* fromSlot = GetFromSlot(ctx);

    if (form.From.Empty()) {
        errorCodeSlot->Value = "unparsed_from";
        errorSlot->Value = fromSlot->Value;
        return AskFromPoint(ctx, form);
    }

    fromSlot->Value = form.From.Get()->GetTitle();

    if (!form.From.Get()->HasAirport()) {
        errorCodeSlot->Value = NO_AIRPORT_ERROR;
        errorSlot->Value = form.From.Get()->GetTitle();
        return AskFromPoint(ctx, form);
    }

    if (form.From.Get()->IsStation()) {
        errorCodeSlot->Value = "from_airport";
        return AskFromPoint(ctx, form);
    }


    TContext::TPtr currentContext = &ctx;

    TSlot* toSlot = GetToSlot(*currentContext);

    if (form.To.Empty()) {
        errorCodeSlot->Value = "unparsed_to";
        errorSlot->Value = toSlot->Value;
        return AskToPoint(*currentContext, form);
    }

    toSlot->Value = form.To.Get()->GetTitle();

    if (!form.To.Get()->HasAirport()) {
        errorCodeSlot->Value = NO_AIRPORT_ERROR;
        errorSlot->Value = form.To.Get()->GetTitle();
        return AskToPoint(*currentContext, form);
    }

    if (form.To.Get()->IsStation()) {
        errorCodeSlot->Value = "to_airport";
        return AskToPoint(*currentContext, form);
    }

    if (ctx.FormName() == AVIA_CHECKOUT_FORM) {
        return Checkout(ctx, form);
    }

    if (ctx.FormName() != AVIA_MAIN_FORM) {
        currentContext = ctx.SetResponseForm(AVIA_MAIN_FORM, false);
        Y_ENSURE(currentContext);
        std::initializer_list<TStringBuf> slots = {
                TStringBuf("geo_from"),
                TStringBuf("geo_to"),
                TStringBuf("date_forward"),
                TStringBuf("date_backward")};
        currentContext->CopySlotsFrom(ctx, slots);
        errorCodeSlot = GetErrorCodeSlot(*currentContext);
    }


    if (!form.DateForward.Empty() || form.MonthNumber != 0) {
       currentContext->GetOrCreateSlot("date_forward", "datetime_raw")->Value = SerializeDateForward(form);
    }

    if (!form.DateBackward.Empty()) {
       currentContext->GetOrCreateSlot("date_backward", "datetime")->Value = ConvertDateTimeToValue(form.DateBackward.GetRef());
    }

    if (form.From == form.To) {
        errorSlot->Value = "same_points";
        errorCodeSlot->Value = "same_points";
        TAviaResolver resolver{*currentContext};
        resolver.AddSuggests(TStringBuf("to"), fromSlot->Value, form.From.GetRef(), 2);
        resolver.AddSuggests(TStringBuf("from"), toSlot->Value, form.To.GetRef(), 2);
        currentContext->AddTextCardBlock("same_points");
        return TResultValue();
    }

    if (form.From.Get()->IsCountry()) {
        errorCodeSlot->Value = "from_country";
        return AskFromPoint(*currentContext, form);
    }


    if (form.To.Get()->IsCountry()) {
        return GenerateToCountryResult(*currentContext, form);
    }

    if (form.DateForward.Empty() || form.MonthNumber != 0) { //  Request without dates or only month is known
        return GenerateDirectionResult(*currentContext, form);
    }

    return GenerateDateResult(*currentContext, form);
}

TResultValue TAviaFormHandler::AskToPoint(TContext& ctx, const NBASS::TAviaForm& form) const {
    TStringBuf newFormName = "personal_assistant.scenarios.avia__ask_to";
    TContext::TPtr newContext = ctx.SetResponseForm(newFormName, false);
    Y_ENSURE(newContext);

    std::initializer_list<TStringBuf> slots = {
        TStringBuf("error_code"),
        TStringBuf("geo_from"),
        TStringBuf("date_forward"),
        TStringBuf("date_backward"),
    };
    newContext->CopySlotsFrom(ctx, slots);

    newContext->CreateSlot("error", "string", true)->Value = ctx.GetSlot("geo_to", "string")->Value;
    newContext->CreateSlot("geo_to", "string", true)->Value = NSc::Null();

    newContext->AddTextCardBlock("ask__to");

    TAviaResolver resolver{*newContext};
    resolver.AddSuggests(TStringBuf("to"), GetFromSlot(ctx)->Value, form.From, 2);
    resolver.AddDefaultCountrySuggest(TStringBuf("to"));

    return TResultValue();
}

TResultValue TAviaFormHandler::AskFromPoint(TContext& ctx, const NBASS::TAviaForm& form) const {
    TAviaResolver resolver{ctx};
    if (GetErrorCodeSlot(ctx)->Value == NO_AIRPORT_ERROR) {
        TVector<TString> titles;
        resolver.FindNearestsCities(form.From->GetAviaId(), 300, true, 3, titles);
        for (const TStringBuf title: titles) {
            LOG(DEBUG) << "Title" << title << Endl;
            resolver.AddSuggestByTitle("from", title);
        }
    } else {
        resolver.AddSuggests(TStringBuf("from"), GetToSlot(ctx)->Value, form.To);
    }

    ctx.AddTextCardBlock("ask__from");
    return TResultValue();
}

TResultValue TAviaFormHandler::GenerateDirectionResult(TContext& ctx, const NBASS::TAviaForm& form) const {
    Y_ASSERT(!form.From.Empty());
    Y_ASSERT(!form.To.Empty());
    TAviaPriceFetcher priceFetcher{ctx};

    TVector<TAviaPriceInfo> minPrices;
    TVector<ui64> fromIds = {form.From.Get()->GetAviaId()};

    i32 minPrice = 0;
    TMaybe<TDateTime::TSplitTime> minPriceDate{};
    if (form.MonthNumber == 0) {
        TVector<ui64> toIds = {form.To.Get()->GetAviaId()};
        priceFetcher.FetchMinPrices(fromIds, toIds, form.DateForward, form.DateBackward, 30, minPrices);
        LOG(DEBUG) << "MinPrices size: " << minPrices.size() << Endl;
        if (minPrices.size() != 0) {
            minPrice = minPrices[0].GetValue();
            minPriceDate = minPrices[0].GetDateForward();
        } else {
            LOG(DEBUG) << "Send init to TDApi" << Endl;
            AddToQueue(new WaitResponse(TAviaTDApiClient(ctx).Init(form)));
        }
    } else {
        ui64 toId = form.To.Get()->GetAviaId();
        priceFetcher.FetchMonthMinPrices(fromIds[0], toId, form.MonthNumber, minPrices);
        LOG(DEBUG) << "MinPrice in month size: " << minPrices.size() << Endl;
        for (const TAviaPriceInfo& priceInfo: minPrices) {
            if ((minPrice == 0) || (priceInfo.GetValue() < minPrice)) {
                minPrice = priceInfo.GetValue();
                minPriceDate = priceInfo.GetDateForward();
            }
        }
    }

    TString link = GenerateDirectionLink(ctx, form, minPriceDate);

    if (minPrice != 0) {
        GetMinPriceSlot(ctx)->Value = minPrice;
        GetMinPriceDateSlot(ctx)->Value = ConvertDateTimeToValue(minPriceDate.GetRef());
    }

    if (ctx.ClientFeatures().SupportsDivCards()) {
        NSc::TValue cardData;
        cardData["geo_from"] = form.From.Get()->GetTitle();
        cardData["geo_to"] = form.To.Get()->GetTitle();
        cardData["dynamic_link"] = link;
        if (minPrice != 0) {
            cardData["min_price"] = minPrice;
            cardData["min_price_date"] = ConvertDateTimeToValue(minPriceDate.GetRef());
            cardData["serp_link"] = GenerateAviaSERPLink(ctx, form, minPriceDate);
            ctx.AddDivCardBlock("avia_direction", std::move(cardData));
            ctx.AddTextCardBlock(TStringBuf("voice_comment"));
        } else {
            AddSearchLinkSuggest(ctx, link);
            ctx.AddTextCardBlock("avia_direction_no_prices", std::move(cardData));
        }
    } else {
        ctx.AddTextCardBlock(TStringBuf("without_cards"));
    }

    TDateTime::TSplitTime userTime = UserTimeFromContext(ctx);
    TDateTime::TSplitTime userDay = GetDate(userTime);
    if (form.MonthNumber == 0) {
        TVector<TAviaDatePair> dates;
        if (minPrice == 0) {
            GenerateDates(form.DateForward, form.DateBackward, userDay, dates);
        } else {
            GenerateDates(minPriceDate, form.DateBackward, userDay, dates);
        }
        AddDateSuggests(ctx, dates);
        AddDefaultMonthSuggests(ctx, userTime);

    } else {
        TVector<i8> months;
        if (form.MonthNumber >= userTime.RealMonth()) {
            if (form.MonthNumber - 1 >= userTime.RealMonth()) {
                months.push_back((form.MonthNumber == 1) ? 12 : form.MonthNumber - 1);
            } else {
                months.push_back((form.MonthNumber + 1) % 12 + 1);
            }
        } else {
            months.push_back((form.MonthNumber == 1) ? 12 : form.MonthNumber - 1);
        }
        months.push_back(form.MonthNumber % 12 + 1);
        AddMonthSuggests(ctx, months);
        TVector<TAviaDatePair> dates;
        GenerateDates(TMaybe<TDateTime::TSplitTime>{}, form.DateBackward, userDay, dates);
        AddDateSuggests(ctx, dates);
    }

    return TResultValue();
}

TString TAviaFormHandler::GenerateDirectionLink(TContext& ctx, const TAviaForm& form, const TMaybe<TDateTime::TSplitTime>& when) const {
    Y_ASSERT(!form.From.Empty());
    Y_ASSERT(!form.To.Empty());
    return GenerateDirectionLink(ctx, form.From.GetRef(), form.To.GetRef(), form.MonthNumber, when);
}


TString TAviaFormHandler::GenerateDirectionLink(NBASS::TContext &ctx, const TAviaPoint& from, const TAviaPoint& to, i8 monthNumber, const TMaybe<TDateTime::TSplitTime>& when) const {
    TCgiParameters cgi;
    cgi.InsertUnescaped("utm_source", GetUtmSource(ctx));
    cgi.InsertUnescaped("utm_medium", "card");
    cgi.InsertUnescaped("utm_campaign", "no_date");

    const TString& iataFrom = from.GetIATA();
    const TString& iataTo = to.GetIATA();

    bool toCountry = to.IsCountry();
    const TString page = toCountry ? "country" : "routes";
    TStringBuilder link;
    link << "https://avia.yandex.ru/" << page << "/"
         << iataFrom << "/" << iataTo << "/"
         << iataFrom << "-" << iataTo << "/";

    if (!toCountry) {
        link << "prices/";
    }

    if (!when.Empty()) {
        cgi.InsertUnescaped("when", when.Get()->ToString("%Y-%m-%d"));
    } else if (monthNumber != 0) {
        TDateTime::TSplitTime userTime = UserTimeFromContext(ctx);

        int userMonth = userTime.RealMonth();
        int year = userTime.RealYear();
        int day = 1;
        if (monthNumber == userMonth) {
            day = userTime.MDay();
        } else if (monthNumber < userMonth) {
            year += 1;
        }
        cgi.InsertEscaped("when", TStringBuilder() << ToString(year) << "-" << ToString(monthNumber) << "-" << ToString(day));
    }

    link << "?" << cgi.Print();

    return link;

}

NSc::TValue TAviaFormHandler::ConvertDateTimeToValue(const TDateTime::TSplitTime& datetime) {
    NSc::TValue result;
    result["years"] = datetime.RealYear();
    result["months"] = datetime.RealMonth();
    result["days"] = datetime.MDay();
    result["hours"] = datetime.Hour();
    result["minutes"] = datetime.Min();
    result["seconds"] = datetime.Sec();
    return result;
}

TResultValue TAviaFormHandler::GenerateDateResult(TContext& ctx, const TAviaForm& form) const {
    Y_ASSERT(!form.From.Empty());
    Y_ASSERT(!form.To.Empty());
    Y_ASSERT(!form.DateForward.Empty());

    TString link = GenerateAviaSERPLink(ctx, form);
    NSc::TValue cardData;
    cardData["geo_from"] = form.From.Get()->GetTitle();
    cardData["geo_to"] = form.To.Get()->GetTitle();
    cardData["serp_link"] = link;
    cardData["date_forward"] = ConvertDateTimeToValue(form.DateForward.GetRef());
    cardData["date_backward"] = form.DateBackward.Empty() ? NSc::Null() : ConvertDateTimeToValue(
            form.DateBackward.GetRef());

    TVector<TAviaPriceInfo> prices;
    TAviaPriceFetcher priceFetcher{ctx};

    TVector<ui64> fromIds = {form.From.Get()->GetAviaId()};
    TVector<ui64> toIds = {form.To.Get()->GetAviaId()};
    priceFetcher.FetchMinPrices(fromIds, toIds, form.DateForward, form.DateBackward, 0, prices);

    if (prices.size() != 0) {
        i32 value = prices[0].GetValue();
        if (value != 0) {
            GetMinPriceSlot(ctx)->Value = value;
            cardData["min_price"] = value;
        }
    } else {
        AddToQueue(new WaitResponse(TAviaTDApiClient(ctx).Init(form)));
    }

    if (ctx.ClientFeatures().SupportsDivCards()) {
        if (prices.size() != 0) {
            ctx.AddDivCardBlock("avia_direction_with_dates", std::move(cardData));
            ctx.AddTextCardBlock(TStringBuf("voice_comment"));
        } else {
            AddSearchLinkSuggest(ctx, link);
            ctx.AddTextCardBlock("avia_direction_with_dates_no_prices", std::move(cardData));
        }
    } else {
        ctx.AddTextCardBlock(TStringBuf("without_cards"));
    }

    TVector<TAviaDatePair> dates;
    TDateTime::TSplitTime userTime = UserTimeFromContext(ctx);

    TDateTime::TSplitTime userDay = GetDate(userTime);
    GenerateDates(form.DateForward, form.DateBackward, userDay, dates);
    AddDateSuggests(ctx, dates);
    AddDefaultMonthSuggests(ctx, userTime);
    return TResultValue();
}


TString TAviaFormHandler::GenerateAviaSERPLink(TContext& ctx, const TAviaForm& form, const TMaybe<TDateTime::TSplitTime>& anotherDate) const {
    Y_ASSERT(!(form.DateForward.Empty() && anotherDate.Empty()));
    const TDateTime::TSplitTime& dateForward = anotherDate.Empty() ? form.DateForward.GetRef() : anotherDate.GetRef();
    return GenerateAviaSERPLink(ctx, form.From.GetRef(), form.To.GetRef(),
        dateForward, form.DateBackward, form.Class, form.Adults, form.Children, form.Infants);
}

TString TAviaFormHandler::GenerateAviaSERPLink(TContext &ctx, const TAviaPoint &from, const TAviaPoint &to,
                                               const TDateTime::TSplitTime& dateForward,
                                               const TMaybe<TDateTime::TSplitTime>& dateBackward,
                                               TAviaTicketClass ticketClass, ui8 adults, ui8 children, ui8 infants) const {
    TCgiParameters cgi;
    cgi.InsertUnescaped("fromId", from.CreateKey());
    cgi.InsertUnescaped("toId", to.CreateKey());
    cgi.InsertUnescaped("when", dateForward.ToString("%Y-%m-%d"));

    if (!dateBackward.Empty()) { // Return ticket
        cgi.InsertUnescaped("return_date", dateBackward.Get()->ToString("%Y-%m-%d"));
    } else {
        cgi.InsertUnescaped("oneway", "1");
    }

    cgi.InsertUnescaped("adult_seats", ToString(adults));
    cgi.InsertUnescaped("children_seats", ToString(children));
    cgi.InsertUnescaped("infant_seats", ToString(infants));
    cgi.InsertUnescaped("klass", ToString(ticketClass));
    cgi.InsertUnescaped("fromBlock", "Alice");

    cgi.InsertUnescaped("utm_source", GetUtmSource(ctx));
    cgi.InsertUnescaped("utm_medium", "card");
    cgi.InsertUnescaped("utm_campaign", "search");
    return TStringBuilder() << "https://avia.yandex.ru/search/result/?" << cgi.Print();
}


TAviaPoint::TAviaPoint()
    : AviaId(0)
    , PointType(TAviaPoint::EPointType::Settlement)
{
}

TAviaPoint::TAviaPoint(ui64 aviaId, EPointType pointType, TStringBuf title, TStringBuf iata, bool hasAirport, const TStringBuf countryTitle)
    : AviaId(aviaId)
    , CountryTitle(countryTitle)
    , IATA(iata)
    , PointType(pointType)
    , Title(title)
    , HasAirport_(hasAirport)
{
}

TString TAviaPoint::CreateKey() const {
    return TStringBuilder() << CreatePrefix() << AviaId;
}

const TString& TAviaPoint::GetIATA() const {
    return IATA;
}

const TString& TAviaPoint::GetTitle() const {
    return Title;
}

ui64 TAviaPoint::GetAviaId() const {
    return AviaId;
}

TAviaPoint::EPointType TAviaPoint::GetPointType() const {
    return PointType;
}

bool TAviaPoint::operator==(const TAviaPoint& other) const {
    return (PointType == other.PointType) && (AviaId == other.AviaId);
}

bool TAviaPoint::IsCountry() const {
    return PointType == EPointType::Country;
}

bool TAviaPoint::IsSettlement() const {
    return PointType == EPointType::Settlement;
}

bool TAviaPoint::IsStation() const {
    return PointType == EPointType::Station;
}


bool TAviaPoint::HasAirport() const {
    return HasAirport_;
}

char TAviaPoint::CreatePrefix() const {
    switch (PointType) {
        case EPointType::Settlement:
            return 'c';
        case EPointType::Station:
            return 's';
        case EPointType::Country:
            return 'l';
    }
}

const TString& TAviaPoint::GetCountryTitle() const {
    return CountryTitle;
}

void TAviaFormHandler::ResetFields(TRequestHandler& r) const {
    GetErrorSlot(r.Ctx())->Value = NSc::Null();
    GetErrorCodeSlot(r.Ctx())->Value = NSc::Null();
    GetMinPriceSlot(r.Ctx())->Value = NSc::Null();
    GetMinPriceDateSlot(r.Ctx())->Value = NSc::Null();
}

TResultValue TAviaFormHandler::GenerateToCountryResult(TContext& ctx, const TAviaForm& form) const {
    Y_ASSERT(!form.From.Empty());
    Y_ASSERT(form.To.Get()->IsCountry());

    TString link = GenerateDirectionLink(ctx, form, form.DateForward);
    TAviaPriceFetcher priceFetcher{ctx};

    TVector<TAviaPriceInfo> minPrices;
    TVector<ui64> fromIds = {form.From.Get()->GetAviaId()};
    LOG(DEBUG) << "Country response" << Endl;
    TVector<TAviaPoint> pointsInCountry;
    TAviaResolver resolver{ctx};
    resolver.FillCountryCities(form, pointsInCountry);
    if (pointsInCountry.size() == 0) {
        GetErrorCodeSlot(ctx)->Value = "no_city_in_country";
        GetErrorSlot(ctx)->Value = form.To.Get()->GetTitle();
        return AskToPoint(ctx, form);
    }

    TVector<ui64> toIds;
    for (const TAviaPoint &city: pointsInCountry) {
        toIds.push_back(city.GetAviaId());
    }
    int windowSize = form.DateForward.Empty() ? 30 : 0;
    priceFetcher.FetchMinPrices(fromIds, toIds, form.DateForward, form.DateBackward, windowSize, minPrices);

    if (ctx.ClientFeatures().SupportsDivCards()) {
        THashMap<ui64, TAviaPriceInfo> pricesById{};
        for (const TAviaPriceInfo& priceInfo: minPrices) {
            pricesById[priceInfo.GetToId()] = priceInfo;
        }

        NSc::TValue cities;
        for (const TAviaPoint& city: pointsInCountry) {
            NSc::TValue cityInfo;
            auto priceInfoIter = pricesById.find(city.GetAviaId());
            if (priceInfoIter == pricesById.end()) { // Price-index does not know a price
                cityInfo["min_price"] = NSc::Null();
                if (form.DateForward.Empty()) {
                    cityInfo["link"] = GenerateDirectionLink(ctx, form.From.GetRef(), city, form.MonthNumber, form.DateForward);
                } else {
                    cityInfo["link"] = GenerateAviaSERPLink(ctx, form.From.GetRef(), city, form.DateForward.GetRef(),
                                                            form.DateBackward, form.Class,
                                                            form.Adults, form.Children, form.Infants);
                }
            } else {
                const TAviaPriceInfo& priceInfo = priceInfoIter->second;
                cityInfo["min_price"] = priceInfo.GetValue();
                cityInfo["link"] = GenerateAviaSERPLink(ctx, form.From.GetRef(), city,
                                                        priceInfo.GetDateForward(), priceInfo.GetDateBackward(),
                                                        form.Class, form.Adults, form.Children, form.Infants);
            }

            cityInfo["title"] = city.GetTitle();
            cities.Push(cityInfo);
        }

        NSc::TValue countryCard;
        countryCard["cities"] = cities;
        countryCard["more_link"] = link;
        countryCard["geo_from"] = form.From.Get()->GetTitle();
        countryCard["geo_to"] = form.To.Get()->GetTitle();
        ctx.AddDivCardBlock("avia_country", countryCard);
        ctx.AddTextCardBlock(TStringBuf("voice_comment"));
    } else {
        ctx.AddTextCardBlock(TStringBuf("without_cards"));
    }

    if (minPrices.size() != 0) {
        i32 minPrice = minPrices[0].GetValue();
        for (const auto& minPriceInfo: minPrices) {
            minPrice = Min(minPrice, minPriceInfo.GetValue());
            GetMinPriceSlot(ctx)->Value = minPrice;
        }
    }

    TDateTime::TSplitTime userTime = UserTimeFromContext(ctx);
    TDateTime::TSplitTime userDay = GetDate(userTime);
    TVector<TAviaDatePair> dates;
    if (form.DateForward.Empty()) {
        for (int step: {1, 2}) {
            TDateTime::TSplitTime dateForward{userDay};
            dateForward.Add(TDateTime::TSplitTime::EField::F_DAY, step);
            dates.push_back({dateForward, TMaybe<TDateTime::TSplitTime>()});
        }
    } else {
        GenerateDates(form.DateForward.GetRef(), form.DateBackward, userDay, dates);
    }
    if (pointsInCountry.size() != 0) {
        resolver.AddSuggestByTitle(TStringBuf("to"), pointsInCountry[0].GetTitle());
    }
    AddDateSuggests(ctx, dates);
    AddDefaultMonthSuggests(ctx, userTime);

    return NBASS::TResultValue();
}

void TAviaFormHandler::AddDateSuggests(TContext& ctx, const TVector<TAviaDatePair>& dates) const {
    for (const TAviaDatePair& datePair: dates) {
        AddDateSuggestVariant(ctx, datePair);
    }
}

void TAviaFormHandler::AddDateSuggestVariant(TContext& ctx, const TAviaFormHandler::TAviaDatePair& datePair) const {
    NSc::TValue dateForward = ConvertDateTimeToValue(datePair.first);
    NSc::TValue json;
    json["date_forward"] = dateForward;
    if (datePair.second.Empty()) {
        json["date_backward"] = NSc::Null();
    } else {
        json["date_backward"] = ConvertDateTimeToValue(datePair.second.GetRef());
    }

    TContext::TSlot dateForwardSlot("date_forward", "datetime_raw");
    dateForwardSlot.Value = dateForward;
    TContext::TSlot dateBackwardSlot("date_backward", "datetime_raw");
    dateBackwardSlot.Value = datePair.second.Empty() ? NSc::Null() : ConvertDateTimeToValue(datePair.second.GetRef());

    NSc::TValue formUpdate;
    formUpdate["name"] = ctx.FormName();
    formUpdate["slots"].SetArray().Push(dateForwardSlot.ToJson(nullptr));
    formUpdate["slots"].Push(dateBackwardSlot.ToJson(nullptr));
    formUpdate["resubmit"].SetBool(true);

    ctx.AddSuggest("avia__dates", std::move(json), std::move(formUpdate));
}

void TAviaFormHandler::AddMonthSuggests(TContext& ctx, const TVector<i8>& months) const {
    for (auto month: months) {
        AddMonthSuggestVariant(ctx, month);
    }
}

void TAviaFormHandler::AddMonthSuggestVariant(TContext &ctx, i8 month) const {
    NSc::TValue json;
    json["month"] = month;

    TContext::TSlot dateForwardSlot("date_forward", "datetime_raw");
    dateForwardSlot.Value.SetDict();
    dateForwardSlot.Value["months"] = month;

    NSc::TValue formUpdate;
    formUpdate["name"] = ctx.FormName();
    formUpdate["slots"].SetArray().Push(dateForwardSlot.ToJson(nullptr));
    formUpdate["resubmit"].SetBool(true);

    ctx.AddSuggest("avia__month", std::move(json), std::move(formUpdate));
}

std::pair<time_t, TMaybe<time_t>> TAviaFormHandler::GetHashPair(const TAviaFormHandler::TAviaDatePair& datePair) const {
    time_t first = datePair.first.AsTimeT();
    TMaybe<time_t> second;
    if (!datePair.second.Empty()) {
        second = datePair.second.Get()->AsTimeT();
    }
    return {first, second};
}

void TAviaFormHandler::GenerateDates(const TMaybe<TDateTime::TSplitTime>& dateForward, const TMaybe<TDateTime::TSplitTime>& dateBackward,
                                     const TDateTime::TSplitTime& userDay, TVector<TAviaDatePair>& out) const {
    out.clear();
    TVector<int> steps;
    const TDateTime::TSplitTime* baseDate = &userDay;
    if (!dateForward.Empty()) {
        baseDate = dateForward.Get();
    }

    if (dateForward.Empty() || (dateForward.GetRef().AsTimeT() == userDay.AsTimeT())) {
        steps.push_back(1);
        steps.push_back(2);
    } else {
        steps.push_back(-1);
        steps.push_back(1);
    }

    for (int step: steps) {
        TDateTime::TSplitTime date = *baseDate;
        date.Add(TDateTime::TSplitTime::EField::F_DAY, step);
        TMaybe<TDateTime::TSplitTime> currentDateBackward = dateBackward;
        if (!dateBackward.Empty()) {
            TDateTime::TSplitTime tmpDate = dateBackward.GetRef();
            tmpDate.Add(TDateTime::TSplitTime::EField::F_DAY, step);
            currentDateBackward = tmpDate;
        }
        out.push_back({date, currentDateBackward});
    }
}

void TAviaFormHandler::AddDefaultMonthSuggests(TContext& ctx, const TDateTime::TSplitTime& userTime) const {
    auto nextMonth = NextMonth(userTime.RealMonth());
    AddMonthSuggests(ctx, {{ nextMonth, NextMonth(nextMonth) }});
}


void TAviaFormHandler::AddSearchLinkSuggest(TContext& ctx, const TStringBuf uri) const {
    NSc::TValue data;
    data["uri"] = uri;
    ctx.AddSuggest("avia__search_link", std::move(data));
}


TString TAviaFormHandler::GenerateCheckoutLink(TContext& ctx, const TAviaForm& form) const {
    if (form.To.GetRef().IsCountry() || form.DateForward.Empty()) {
        TVector<TAviaPriceInfo> minPrices;
        TAviaPriceFetcher priceFetcher{ctx};
        auto fromId = form.From.Get()->GetAviaId();
        auto toId = form.To.Get()->GetAviaId();
        if (form.MonthNumber) {
            priceFetcher.FetchMonthMinPrices(fromId, toId,form.MonthNumber, minPrices);
        } else {
            priceFetcher.FetchMinPrices({ fromId }, { toId }, form.DateForward, form.DateBackward, 30, minPrices);
        }

        TDateTime::TSplitTime when = (minPrices.size() == 0) ? UserTimeFromContext(ctx) : minPrices[0].GetDateForward();

        return GenerateDirectionLink(ctx, form, when);
    }

    return GenerateAviaSERPLink(ctx, form);
}

TResultValue TAviaFormHandler::Checkout(TContext& ctx, const TAviaForm& form) const {
    if (ctx.ClientFeatures().SupportsDivCards()) {
        ctx.AddTextCardBlock(TStringBuf("avia__checkout"));
        NSc::TValue data;
        TString link = GenerateCheckoutLink(ctx, form);
        data["uri"] = link;
        ctx.AddSuggest(TStringBuf("avia__checkout_link"), data);
        ctx.AddCommand<TAviaCheckoutSiteDirective>(TStringBuf("open_uri"), data);
    } else {
        ctx.AddTextCardBlock(TStringBuf("avia__checkout_text"));
    }

    return NBASS::TResultValue();
}

NSc::TValue TAviaFormHandler::SerializeDateForward(const TAviaForm& form) {
    if (form.MonthNumber != 0) {
        NSc::TValue result;
        result["months"] = form.MonthNumber;
        return result;
    }

    if (!form.DateForward.Empty()) {
        return ConvertDateTimeToValue(form.DateForward.GetRef());
    }

    return NSc::Null();
}

void TAviaFormHandler::AddToQueue(IObjectInQueue* obj) const {
    if (!ThreadPool.AddAndOwn(THolder(obj))) {
        LOG(ERR) << "Can not put object in the queue (queue is full)" << Endl;
    };
}

}
