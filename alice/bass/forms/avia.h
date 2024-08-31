#pragma once

#include "vins.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/library/datetime/datetime.h>

#include <util/generic/cast.h>
#include <util/generic/maybe.h>
#include <util/string/builder.h>

namespace NBASS {
class TAviaPoint {
public:
    enum class EPointType {
        Settlement,
        Station,
        Country
    };

public:
    TAviaPoint();

    TAviaPoint(ui64 aviaId, EPointType pointType, const TStringBuf title, const TStringBuf iata, bool hasAirport, const TStringBuf countryTitle);

    ui64 GetAviaId() const;
    TAviaPoint::EPointType GetPointType() const;
    const TString& GetTitle() const;
    const TString& GetIATA() const;
    const TString& GetCountryTitle() const;

    TString CreateKey() const;

    bool operator==(const TAviaPoint& other) const;
    bool IsCountry() const;
    bool IsSettlement() const;
    bool IsStation() const;
    bool HasAirport() const;

private:
    ui64 AviaId;
    TString CountryTitle;
    TString IATA;
    EPointType PointType;
    TString Title;
    bool HasAirport_;

    char CreatePrefix() const;
};

enum class TAviaTicketClass {
    Economy /* "economy" */,
    Business /* "business" */
};

struct TAviaForm {
    TMaybe<TAviaPoint> From;
    TMaybe<TAviaPoint> To;
    ui8 Adults = 1;
    ui8 Children = 0;
    ui8 Infants = 0;
    TMaybe<NAlice::TDateTime::TSplitTime> DateForward;
    TMaybe<NAlice::TDateTime::TSplitTime> DateBackward;
    TAviaTicketClass Class = TAviaTicketClass::Economy;
    i8 MonthNumber = 0;
};

class TAviaFormHandler: public IHandler {
public:
    TAviaFormHandler(IThreadPool& threadPool);
    static void Register(THandlersMap* handlersMap, IGlobalContext& globalCtx);
    TResultValue Do(TRequestHandler& r) override;

private:
    using TAviaDatePair = std::pair<NAlice::TDateTime::TSplitTime, TMaybe<NAlice::TDateTime::TSplitTime>>;
    void AddToQueue(IObjectInQueue*) const;
    void AddSearchLinkSuggest(TContext& ctx, const TStringBuf uri) const;
    void AddDateSuggests(TContext& ctx, const TVector<TAviaDatePair>& dates) const;
    void AddDateSuggestVariant(TContext& ctx, const TAviaDatePair& datePair) const;
    void AddDefaultMonthSuggests(TContext& ctx, const NAlice::TDateTime::TSplitTime& userTime) const;
    void AddMonthSuggests(TContext& ctx, const TVector<i8>& months) const;
    void AddMonthSuggestVariant(TContext& ctx, i8 month) const;
    TResultValue Checkout(TContext& ctx, const TAviaForm& form) const;
    TResultValue AddErrorMessage(TContext& ctx) const;
    TResultValue AskFromPoint(TContext& ctx, const TAviaForm& form) const;
    TResultValue AskToPoint(TContext& ctx, const TAviaForm& form) const;
    TString GenerateCheckoutLink(TContext& ctx, const TAviaForm& form) const;
    TResultValue GenerateToCountryResult(TContext& ctx, const TAviaForm& form) const;
    TResultValue GenerateDateResult(TContext& ctx, const TAviaForm& form) const;
    TResultValue GenerateDirectionResult(TContext& ctx, const TAviaForm& form) const;
    TResultValue GenerateResult(TRequestHandler& r, const TAviaForm& form) const;
    TString GenerateDirectionLink(TContext& ctx, const TAviaForm& form, const TMaybe<NAlice::TDateTime::TSplitTime>& when) const;
    TString GenerateDirectionLink(NBASS::TContext &ctx, const TAviaPoint& from, const TAviaPoint& to, i8 monthNumber, const TMaybe<NAlice::TDateTime::TSplitTime>& when) const;
    static NSc::TValue ConvertDateTimeToValue(const NAlice::TDateTime::TSplitTime& datetime);
    TString GenerateAviaSERPLink(TContext& ctx, const TAviaForm& form, const TMaybe<NAlice::TDateTime::TSplitTime>& anotherDate=TMaybe<NAlice::TDateTime::TSplitTime>()) const;
    TString GenerateAviaSERPLink(TContext& ctx, const TAviaPoint& from, const TAviaPoint& to, const NAlice::TDateTime::TSplitTime& dateForward, const TMaybe<NAlice::TDateTime::TSplitTime>& dateBackward,
                                 TAviaTicketClass ticketClass, ui8 adults, ui8 children, ui8 infants) const;
    std::pair<time_t, TMaybe<time_t>> GetHashPair(const TAviaDatePair& datePair) const;
    void GenerateDates(const TMaybe<NAlice::TDateTime::TSplitTime>& dateForward, const TMaybe<NAlice::TDateTime::TSplitTime>& dateBackward, const NAlice::TDateTime::TSplitTime& userDay, TVector<TAviaDatePair>& out) const;

    void ResetFields(TRequestHandler& r) const;
    static NSc::TValue SerializeDateForward(const TAviaForm& form);

private:
    IThreadPool& ThreadPool;
};

}
