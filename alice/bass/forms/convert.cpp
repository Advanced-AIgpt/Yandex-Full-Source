#include "convert.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/datetime/datetime.h>
#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/cgiparam/cgiparam.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS {

namespace {

constexpr TStringBuf SUGGEST_USD_RATE = "USD";
constexpr TStringBuf SUGGEST_EUR_RATE = "EUR";

constexpr TStringBuf DEFAULT_TLD = "ru";

enum EConverterErrors {
    INCORRECT_DATE = 1, // неправильно задали дату (неверный формат даты)
    NO_SOURCE_FOR_INPUT = 2, // невозможно найти источник для входных параметров (сейчас это: src, региону, по названию котировок)
    NO_RATE_FOR_INPUT = 3, // редкая ошибка (все параметры правильные, но не смогли получить котировку, скорее всего(!), нет котировки за дату)
    BAD_LANGUAGE = 4, // задан неподдерживаемый язык
    CANT_FIND_TO_FOR_INPUT = 5, // не задан параметр "to", но мы не смогли его определить (по src, региону)
    NO_RATE_IN_REGION = 6, // нет котировки для региона
    NO_RATE_FOR_DATE = 7, // нет котировки для даты
    NO_RATE_FOR_SOURCE = 8, // у данного источника нет такой пары валют
    UNKNOWN_ERROR = 128 // возникла какя-то непредвиденная ошибка
};

enum EConverterErrorTypes {
    OK = 0,
    SYSTEM = 1,
    CONVERTERROR = 2,
};

EConverterErrorTypes ConvertErrorTextFromCode(int n, TString* code) {
    EConverterErrorTypes errorType = EConverterErrorTypes::SYSTEM;
    switch(n) {
        case EConverterErrors::INCORRECT_DATE:
            *code = TStringBuf("no_rate_for_date");
            errorType = EConverterErrorTypes::CONVERTERROR;
            break;
        case EConverterErrors::NO_SOURCE_FOR_INPUT:
            *code = TStringBuf("no_source_for_input");
            errorType = EConverterErrorTypes::CONVERTERROR;
            break;
        case EConverterErrors::NO_RATE_FOR_INPUT:
            *code = TStringBuf("no_rate_for_input");
            errorType = EConverterErrorTypes::CONVERTERROR;
            break;
        case EConverterErrors::BAD_LANGUAGE:
            *code = TStringBuf("bad_language");
            errorType = EConverterErrorTypes::SYSTEM;
            break;
        case EConverterErrors::CANT_FIND_TO_FOR_INPUT:
            *code = TStringBuf("cant_find_to_for_input");
            errorType = EConverterErrorTypes::CONVERTERROR;
            break;
        case EConverterErrors::NO_RATE_IN_REGION:
            *code = TStringBuf("no_rate_in_region");
            errorType = EConverterErrorTypes::CONVERTERROR;
            break;
        case EConverterErrors::NO_RATE_FOR_DATE:
            *code = TStringBuf("no_rate_for_date");
            errorType = EConverterErrorTypes::CONVERTERROR;
            break;
        case EConverterErrors::NO_RATE_FOR_SOURCE:
            *code = TStringBuf("no_rate_for_source");
            errorType = EConverterErrorTypes::CONVERTERROR;
            break;
        case EConverterErrors::UNKNOWN_ERROR:
            *code = TStringBuf("unknown_error");
            errorType = EConverterErrorTypes::SYSTEM;
            break;
        default:
            *code = TStringBuf("unknown_error_code");
            errorType = EConverterErrorTypes::SYSTEM;
            break;
    }
    return errorType;
}

}

class TConvertRequestImpl {
public:
    TConvertRequestImpl(TRequestHandler& r);
    TResultValue Do();

private:
    TContext& Ctx;

    TRequestedGeo Geo;
    TString TypeFrom;
    TString TypeTo;
    TString Source;
    TString When;
    double AmountFrom = 0;
    double AmountTo = 0;

    bool HasWhen = false;
    bool RemovedWhen = false;

    bool HasSource = false;
    bool RemovedSource = false;

    bool HasWhere = false;
    bool ChangedWhere = false;

    bool ProcessInputSlots();
    void ProcessDate(TResultValue* dateError);
    void GetRegionInfo(NSc::TValue* info) const;
    TResultValue MakeCgiRequest(const NSc::TValue& patch, NSc::TValue* converterResult) const;
    EConverterErrorTypes GetAnswer(const NSc::TValue& patch, NSc::TValue* converterResult, TString* code) const;
    TResultValue FillOutputSlots(NSc::TValue& converterResult);
    TResultValue ProcessAmount(NSc::TValue& converterResult, TStringBuf direction, double* amountNum);
    bool CheckSuggestedRequest(TStringBuf fieldName, TStringBuf fieldValue) const;
};

TConvertRequestImpl::TConvertRequestImpl(TRequestHandler& r)
    : Ctx(r.Ctx())
    , Geo(Ctx.GlobalCtx())
{
}

bool TConvertRequestImpl::ProcessInputSlots() {
    TContext::TSlot* slotTypeFrom = Ctx.GetSlot("type_from");
    TContext::TSlot* slotTypeTo = Ctx.GetOrCreateSlot("type_to", "currency");
    TContext::TSlot* slotAmountFrom = Ctx.GetOrCreateSlot("amount_from", "num");
    TContext::TSlot* slotSource = Ctx.GetOrCreateSlot("source", "currency_conv_source");
    TContext::TSlot* slotWhere = Ctx.GetSlot("where");

    if (IsSlotEmpty(slotTypeFrom)) {
        Ctx.CreateSlot("type_from", "currency", false);
        return false;
    } else {
        TypeFrom = slotTypeFrom->Value.GetString();
    }

    if (!IsSlotEmpty(slotTypeTo)) {
        TypeTo = slotTypeTo->Value.GetString();
        if (!TypeTo.empty() && TypeTo == TypeFrom) {
            NSc::TValue er;
            er["code"] = TStringBuf("cant_find_to_for_input");
            Ctx.AddErrorBlock(
                TError(
                    TError::EType::CONVERTERROR,
                    TStringBuf("cant_find_to_for_input")
                ),
                std::move(er)
            );
            return false;
        }
    }

    if (!slotAmountFrom->Value.IsNull()) {
        AmountFrom = slotAmountFrom->Value.GetNumber();
        if (AmountFrom <= 0) {
            NSc::TValue er;
            er["code"] = TStringBuf("non_positive_amount_from");
            Ctx.AddErrorBlock(
                TError(
                    TError::EType::INVALIDPARAM,
                    TStringBuf("non_positive_amount_from")
                ),
                std::move(er)
            );
            return false;
        }
    }

    if (!IsSlotEmpty(slotWhere)) {
        HasWhere = true;
    }
    Geo = TRequestedGeo(Ctx, slotWhere);
    if (Geo.HasError()) {
        // do not fail with empty geolocation
        if (Geo.GetError()->Type != TError::EType::NOUSERGEO) {
            NSc::TValue er;
            er["code"] = TStringBuf("bad_geo");
            Ctx.AddErrorBlock(*Geo.GetError(), std::move(er));
            return false;
        }
    } else {
        if (Geo.GetGeoType() > NGeobase::ERegionType::VILLAGE) {
            // find greater area
            NGeobase::TId parentId = Geo.GetParentCityId();
            if (!NAlice::IsValidId(parentId)) {
                parentId = Geo.GetParentIdByType(NGeobase::ERegionType::COUNTRY);
            }
            if (!NAlice::IsValidId(parentId)) {
                NSc::TValue er;
                er["code"] = TStringBuf("bad_geo");
                Ctx.AddErrorBlock(TError::EType::NOGEOFOUND, std::move(er));
                return false;
            }
            Geo.ConvertTo(parentId);
            ChangedWhere = true;
        }
        NSc::TValue info;
        GetRegionInfo(&info);
        Ctx.CreateSlot(TStringBuf("resolved_where"), TStringBuf("geo"), true, info);
    }

    TResultValue dateError;
    ProcessDate(&dateError);
    if (dateError) {
        NSc::TValue er;
        er["code"] = TStringBuf("bad_date");
        Ctx.AddErrorBlock(*dateError, std::move(er));
        return false;
    }

    if (!slotSource->Value.IsNull()) {
        Source = slotSource->Value.GetString();
        HasSource = true;
    }

    return true;
}

void TConvertRequestImpl::ProcessDate(TResultValue* dateError) {
    TContext::TSlot* slotWhen = Ctx.GetSlot("when");

    if (!IsSlotEmpty(slotWhen)) {
        const TDateTime userTime(
            TDateTime::TSplitTime(
                NDatetime::GetTimeZone(Ctx.UserTimeZone()),
                Ctx.Meta().Epoch()
            )
        );
        try {
            std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TContext::TSlot>(
                slotWhen,
                nullptr,
                userTime,
                { 1 }
            );
            if (dtl && !dtl->IsNow() && dtl->TotalDays() == 1) {
                const TDateTime& dt = *dtl->cbegin();
                When = dt.SplitTime().ToString("%Y-%m-%d");
                HasWhen = true;
            }
        } catch (const yexception& e) {
            *dateError = TError(TError::EType::INVALIDPARAM, e.what());
        }
    }
}

TResultValue TConvertRequestImpl::MakeCgiRequest(const NSc::TValue& patch, NSc::TValue* converterResult) const {
    TCgiParameters cgi;
    cgi.InsertEscaped(TStringBuf("uil"), Ctx.MetaLocale().Lang);
    cgi.InsertEscaped(TStringBuf("geoid"), Geo.IsValidId() ? ToString(Geo.GetId()) : TStringBuf("0"));
    cgi.InsertEscaped(TStringBuf("date"), patch["when"].IsNull() ? When : patch["when"].GetString());
    cgi.InsertEscaped(TStringBuf("from"), patch["type_from"].IsNull() ? TypeFrom : patch["type_from"].GetString());
    cgi.InsertEscaped(TStringBuf("to"), patch["type_to"].IsNull() ? TypeTo : patch["type_to"].GetString());
    cgi.InsertEscaped(TStringBuf("src"), patch["source"].IsNull() ? Source : patch["source"].GetString());
    cgi.InsertEscaped(TStringBuf("tld"), HasWhere ? TStringBuf("") : DEFAULT_TLD);

    double amount = patch["amount_from"].IsNull() ? AmountFrom : patch["amount_from"].ForceNumber();
    cgi.InsertEscaped(TStringBuf("fromamount"), amount == 0 ? TStringBuf("") : Sprintf("%f", amount));

    NHttpFetcher::TRequestPtr req = Ctx.GetSources().Convert().Request();
    req->AddCgiParams(cgi);
    LOG(DEBUG) << TStringBuf("search request: ") << req->Url() << Endl;

    NHttpFetcher::THandle::TRef h = req->Fetch();
    NHttpFetcher::TResponse::TRef resp = h->Wait();

    if (resp->IsError()) {
        return TError(
            TError::EType::SYSTEM,
            resp->GetErrorText()
        );
    }

    if (!NSc::TValue::FromJson(*converterResult, resp->Data) || !(*converterResult).IsDict()) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("parsing_error")
        );
    }
    LOG(DEBUG) << TStringBuf("search response: ") << resp->Data << Endl;
    return TResultValue();
}

EConverterErrorTypes TConvertRequestImpl::GetAnswer(const NSc::TValue& patch, NSc::TValue* converterResult, TString* code) const {
    TResultValue requestError = MakeCgiRequest(patch, converterResult);
    if (requestError) {
        return EConverterErrorTypes::SYSTEM;
    }

    if ((*converterResult)["status"].GetString() == TStringBuf("ok")) {
        return EConverterErrorTypes::OK;
    }

    return ConvertErrorTextFromCode((*converterResult)["error_code"].ForceIntNumber(), code);
}

TResultValue TConvertRequestImpl::ProcessAmount(NSc::TValue& converterResult, TStringBuf direction, double* amountNum) {
    if (converterResult[direction].IsNull() || !converterResult[direction].IsDict()) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("parsing_error")
        );
    }

    NSc::TValue& amountToValue(converterResult[direction]["amount"]);
    if (amountToValue.IsNumber()) {
        *amountNum = amountToValue.GetNumber();
    } else if (amountToValue.IsString()) {
        if (!TryFromString(amountToValue.GetString(), *amountNum)) {
            return TError(
                TError::EType::SYSTEM,
                TStringBuf("parsing_error")
            );
        }
    } else {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("parsing_error")
        );
    }

    if (*amountNum > 1e+12) { // we choose this constant in ASSISTANT-651 for
        *amountNum = 0;       // do not synthesize 'trillions', just saying 'very big amount' :)
        Ctx.AddAttention("big_amount", NSc::Null());
    }
    return TResultValue();
}

TResultValue TConvertRequestImpl::FillOutputSlots(NSc::TValue& converterResult) {
    TString oldSource(Source);
    if (converterResult["source"].IsDict()) {
        Source = converterResult["source"]["id"].GetString();
    }
    if (!Source) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("Incorrect converter answer: source of data is absent")
        );
    }
    if (HasSource && oldSource != Source && oldSource != TStringBuf("SOME_BANK") && oldSource != TStringBuf("SOME_EXCHANGE")) {
        HasSource = false;
        RemovedSource = true;
    }
    Ctx.GetSlot("source")->Value.SetString(Source);

    if (TStringBuf("EXCHANGE_POINT") == Source) {
        double amountSell = 0;
        double amountBuy = 0;
        TResultValue amountSellError = ProcessAmount(converterResult, TStringBuf("sell_info"), &amountSell);
        if (amountSellError) {
            return amountSellError;
        }
        TResultValue amountBuyError = ProcessAmount(converterResult, TStringBuf("buy_info"), &amountBuy);
        if (amountBuyError) {
            return amountBuyError;
        }

        NSc::TValue amountInfo;
        amountInfo["sell"] = amountSell;
        amountInfo["buy"] = amountBuy;
        Ctx.CreateSlot("amount_to", "buy_sell_amount", true, std::move(amountInfo));
    } else {
        TResultValue amountToError = ProcessAmount(converterResult, TStringBuf("to"), &AmountTo);
        if (amountToError) {
            return amountToError;
        }
        Ctx.CreateSlot("amount_to", "num", true, AmountTo);
    }

    TContext::TSlot* slotTypeTo = Ctx.GetSlot("type_to");
    if (slotTypeTo->Value.IsNull()) {
        TStringBuf outputTypeTo = converterResult["to"]["id"].GetString();
        if (!outputTypeTo) {
            return TError(
                TError::EType::SYSTEM,
                TStringBuf("parsing_error")
            );
        }
        slotTypeTo->Value.SetString(outputTypeTo);
    }

    if (AmountFrom == 0) {
        double outputAmountFrom = 0;
        TResultValue amountFromError = ProcessAmount(converterResult, TStringBuf("from"), &outputAmountFrom);
        if (amountFromError) {
            return amountFromError;
        }
        Ctx.CreateSlot("amount_base", "num", true, outputAmountFrom);
    }

    TStringBuf outputDate = converterResult["date"].GetString();
    TStringBuf outputTime = converterResult["time"].GetString();
    TStringBuf outputTimezone = converterResult["timezone"]["name"].GetString();
    TContext::TSlot* slotDate = Ctx.CreateSlot("source_date", "string", true);
    TContext::TSlot* slotTimezone = Ctx.CreateSlot("source_timezone", "string", true);

    if (!outputDate) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("Incorrect converter answer: date is absent")
        );
    }
    TStringBuilder outputDateTime;
    outputDateTime << outputDate;
    if (outputTime) {
        outputDateTime << "T" << outputTime << ":00";
    }
    slotDate->Value.SetString(outputDateTime);

    if (outputTimezone) {
        slotTimezone->Value.SetString(outputTimezone);
    }

    // we might want to access converter result directly
    Ctx.CreateSlot("search_response", "search_response", true, std::move(converterResult));

    // suggest blocks
    Ctx.AddSuggest(TStringBuf("convert__describe_rate"));

    // another currency
    if (TypeFrom != TStringBuf("USD") && TypeTo != TStringBuf("USD")) {
        if (CheckSuggestedRequest(TStringBuf("type_from"), TStringBuf("USD"))) {
            NSc::TValue suggest;
            suggest["value"] = SUGGEST_USD_RATE;
            Ctx.AddSuggest(TStringBuf("convert__from"), std::move(suggest));
        }
    }
    if (TypeFrom != TStringBuf("EUR") && TypeTo != TStringBuf("EUR")) {
        if (CheckSuggestedRequest(TStringBuf("type_from"), TStringBuf("EUR"))) {
            NSc::TValue suggest;
            suggest["value"] = SUGGEST_EUR_RATE;
            Ctx.AddSuggest(TStringBuf("convert__from"), std::move(suggest));
        }
    }
    // another source
    if (Source == TStringBuf("RUS") && oldSource != TStringBuf("MOSCOW_EXCHANGE")) {
        if (CheckSuggestedRequest(TStringBuf("source"), TStringBuf("MOSCOW_EXCHANGE"))) {
            NSc::TValue suggest;
            suggest["value"] = TStringBuf("MOSCOW_EXCHANGE");
            Ctx.AddSuggest(TStringBuf("convert__source"), std::move(suggest));
        }
    }
    if (Source == TStringBuf("MOSCOW_EXCHANGE") && oldSource != TStringBuf("RUS")) {
        if (CheckSuggestedRequest(TStringBuf("source"), TStringBuf("RUS"))) {
            NSc::TValue suggest;
            suggest["value"] = TStringBuf("RUS");
            Ctx.AddSuggest(TStringBuf("convert__source"), std::move(suggest));
        }
    }

    // attention blocks
    if (RemovedWhen || RemovedSource) {
        NSc::TValue removed;
        removed["slots"].SetDict();
        if (RemovedWhen) {
            Ctx.CreateSlot("when", "datetime", true);
            removed["slots"]["when"] = When;
        }
        if (RemovedSource) {
            removed["slots"]["source"] = oldSource;
        }

        Ctx.AddAttention("converter_removed_slots", std::move(removed));
    }

    if (ChangedWhere) {
        Ctx.AddAttention("converter_used_parent_location", NSc::Null());
    }

    return TResultValue();
}

bool TConvertRequestImpl::CheckSuggestedRequest(TStringBuf fieldName, TStringBuf fieldValue) const {
    NSc::TValue patch;
    patch[fieldName] = fieldValue;
    if (RemovedWhen) {
        patch["when"] = "";
    }

    NSc::TValue tmpResult;
    TString tmpCode;
    LOG(DEBUG) << "Check suggest: " << fieldName << "=" << fieldValue << Endl;
    EConverterErrorTypes eType = GetAnswer(patch, &tmpResult, &tmpCode);
    return (eType == EConverterErrorTypes::OK);
}

TResultValue TConvertRequestImpl::Do() {
    if (!ProcessInputSlots()) {
        return TResultValue();
    }

    NSc::TValue converterResult;
    NSc::TValue patch;
    TString code;
    size_t eType = 0;

    while (1) {
        converterResult = NSc::Null();

        eType = GetAnswer(patch, &converterResult, &code);

        if (eType != EConverterErrorTypes::CONVERTERROR) {
            break;
        }

        if (HasWhen) {
            if (HasSource && converterResult["error_code"].ForceIntNumber() == EConverterErrors::NO_RATE_FOR_SOURCE) {
                HasSource = false;
                RemovedSource = true;
                patch["source"] = TStringBuf("");
            } else if (!RemovedSource) {
                HasWhen = false;
                RemovedWhen = true;
                patch["when"] = TStringBuf("");
            } else {
                break;
            }
        } else if (HasSource && !RemovedWhen) {
            HasSource = false;
            RemovedSource = true;
            patch["source"] = TStringBuf("");
        } else {
            break;
        }
    }

    if (eType == EConverterErrorTypes::SYSTEM) {
        return TError(
            TError::EType::SYSTEM,
            code
        );
    } else if (eType != EConverterErrorTypes::OK) {
        NSc::TValue er;
        er["code"] = code;
        Ctx.AddErrorBlock(
            TError(
                TError::EType::CONVERTERROR,
                converterResult["error"].GetString()
            ),
            std::move(er)
        );
        return TResultValue();
    }

    if (TResultValue critical = FillOutputSlots(converterResult)) {
        return critical;
    }

    if (Ctx.MetaClientInfo().IsYaAuto()) {
        Ctx.AddStopListeningBlock();
    }

    return TResultValue();
}

void TConvertRequestImpl::GetRegionInfo(NSc::TValue* info) const {
    if (Geo.IsValidId()) {
        Geo.AddAllCaseForms(Ctx, info);
        (*info)["timezone"] = Geo.GetTimeZone();
    }
}

TResultValue TConvertFormHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::CONVERT);
    TConvertRequestImpl impl(r);
    TResultValue result = impl.Do();
    r.Ctx().AddSearchSuggest();
    r.Ctx().AddOnboardingSuggest();
    return result;
}

void TConvertFormHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TConvertFormHandler>();
    };

    handlers->emplace(TStringBuf("personal_assistant.scenarios.convert"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.convert__ellipsis"), handler);
}

}
