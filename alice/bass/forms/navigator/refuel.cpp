#include "refuel.h"

#include <alice/bass/forms/market/util/amount.h>
#include <alice/bass/forms/tanker.sc.h>
#include <alice/bass/forms/route.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/string/cast.h>
#include <util/string/subst.h>
#include <util/string/split.h>

#include <alice/library/analytics/common/product_scenarios.h>

/*
Vins Form Slots:
  "column": 1..100,
  "fuel_type": ["a92", "a95", "a98", "a95_premium, "diesel", etc.],
  "volume": (num of liters/money or "full" of type "tank_volume"),
  "volume_type": ["liter", "rub", "tank"],
  "confirmation": bool,
*/

namespace {

constexpr TStringBuf EMPTY_STR;

struct TFuel {
    TFuel(const TString& id, const TString& fuelType, const TString& name)
        : FuelId(id)
        , FuelType(fuelType)
        , Name(name)
    {}

    TString FuelId; // "a92"
    TString FuelType; // "АИ-92"
    TString Name; // Additional classifier, e.g. "G-Drive" or empty
};
using TGasStation = TMap<i32, TVector<TFuel>>;

struct TOrderRange {
    double Min;
    double Max;
};

struct TTankerInfo {
    TGasStation GasStation;
    TString GasStationId;
    TString GasStationName;
    TString UserFuelId;
    bool HasPaymentMethod;
    TMaybe<TOrderRange> MoneyRange;
    TMaybe<TOrderRange> VolumeRange;
};

TMaybe<TOrderRange> ParseOrderRange(const NBASSTanker::TTankerResponse<TSchemeTraits>::TRange& range) {
    if (range.IsNull() || range.Min().IsNull() || range.Max().IsNull())
        return Nothing();
    double minValue = range.Min();
    double maxValue = range.Max();
    return TOrderRange{ minValue, maxValue };
}

TMaybe<TTankerInfo> FetchTankerInfo(NBASS::TContext& ctx) {
    NHttpFetcher::TRequestPtr req = ctx.GetSources().TankerApi().Request();
    // Add O-Auth token header.
    if (ctx.IsAuthorizedUser()) {
        TString dummy, authToken;
        StringSplitter(ctx.UserAuthorizationHeader()).Split(' ').CollectInto(&dummy, &authToken);
        req->AddHeader(TStringBuf("X-OauthToken"), authToken);
    }
    req->AddHeader(TStringBuf("X-Robot"), TStringBuf("BASS"));
    const auto& tanker = ctx.Meta().DeviceState().Tanker();
    if (!tanker.IsNull() && !tanker.XPayment().IsNull()) {
        TStringBuf xPayment = tanker.XPayment();
        if (!xPayment.empty())
            req->AddHeader(TStringBuf("X-Payment"), xPayment);
    }

    // Fill in request params.
    TCgiParameters cgi;
    cgi.InsertUnescaped("lat", ToString(ctx.Meta().Location().Lat()));
    cgi.InsertUnescaped("lon", ToString(ctx.Meta().Location().Lon()));
    req->AddCgiParams(cgi);

    NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();
    // Parse response.
    if (resp->IsError()) {
        LOG(ERR) << "Tanker API fetch error: " << resp->GetErrorText() << Endl;
        return Nothing();
    }

    LOG(DEBUG) << "Tanker response: " << resp->Data << Endl;

    NSc::TValue value;
    if (!NSc::TValue::FromJson(value, resp->Data))
        return Nothing();

    NBASSTanker::TTankerResponse<TSchemeTraits> tankerScheme(&value);
    auto validate = [](TStringBuf path, TStringBuf error) {
                        LOG(ERR) << "Error parsing tanker response: " << path << " : " << error << Endl;
                    };
    if (!tankerScheme.Validate("" /* path */ , false /* strict */, validate))
        return Nothing();

    if (tankerScheme.Station().IsNull() || tankerScheme.Station().Id().IsNull())
        return Nothing();

    TGasStation stationInfo;

    TMap<TString, TFuel> fuelInfo;
    for (const auto& fuel : tankerScheme.Station().Fuels()) {
        const TString fuelId(fuel.Id());
        fuelInfo.emplace(fuelId, TFuel{ fuelId, TString(fuel.Marka().Get(EMPTY_STR)), TString(fuel.Name().Get(EMPTY_STR)) });
    }

    for (const auto& columnIt : tankerScheme.Station().Columns()) {
        TVector<TFuel> fuels;
        for (const auto& fuelId : columnIt.Value().Fuels()) {
            auto it = fuelInfo.find(fuelId);
            if (it != fuelInfo.end())
                fuels.push_back(it->second);
        }
        stationInfo.emplace(columnIt.Key(), std::move(fuels));
    }

    TTankerInfo tankerInfo;
    tankerInfo.GasStation = std::move(stationInfo);
    tankerInfo.GasStationId = TString(tankerScheme.Station().Id().Get(EMPTY_STR));
    tankerInfo.GasStationName = TString(tankerScheme.Station().Name().Get(EMPTY_STR));
    tankerInfo.UserFuelId = TString(tankerScheme.UserFuelId().Get(EMPTY_STR));
    tankerInfo.HasPaymentMethod = !tankerScheme.Payment().IsNull();
    if (!tankerScheme.OrderRange().IsNull()) {
        tankerInfo.MoneyRange = ParseOrderRange(tankerScheme.OrderRange().Money());
        tankerInfo.VolumeRange = ParseOrderRange(tankerScheme.OrderRange().Litre());
    }
    return tankerInfo;
}

} // namespace


namespace NBASS {

namespace {
// Intents
constexpr TStringBuf NAVI_REFUEL_FORM_NAME = "personal_assistant.navi.refuel";
constexpr TStringBuf NAVI_REFUEL_ELLIPSIS_FORM_NAME = "personal_assistant.navi.refuel__ellipsis";
// Slot names
constexpr TStringBuf SLOT_COLUMN_NAME = "column";
constexpr TStringBuf SLOT_FUEL_TYPE_NAME = "fuel_type";
constexpr TStringBuf SLOT_VOL_NAME = "volume";
constexpr TStringBuf SLOT_VOL_TYPE_NAME = "volume_type";
constexpr TStringBuf SLOT_CONFIRMATION_NAME = "confirmation";
constexpr TStringBuf SLOT_STATION_NAME_NAME = "station_name";
// Slot types
constexpr TStringBuf SLOT_COLUMN_TYPE = "num";
constexpr TStringBuf SLOT_FUEL_TYPE_TYPE = "fuel_type";
constexpr TStringBuf SLOT_VOL_TYPE = "num";
constexpr TStringBuf SLOT_VOL_TYPE_TYPE = "volume_type";
constexpr TStringBuf SLOT_CONFIRMATION_TYPE = "bool";
constexpr TStringBuf SLOT_STATION_NAME_TYPE = "string";
// Slot Values
constexpr TStringBuf SLOT_VOL_TYPE_LITER = "liter";
constexpr TStringBuf SLOT_VOL_TYPE_RUB = "rub";
constexpr TStringBuf SLOT_VOL_TYPE_TANK = "tank";
constexpr TStringBuf SLOT_CONF_YES = "yes";
// VINS to Tanker mapping
constexpr TStringBuf TANKER_VOL_TYPE_LITER = "2";
constexpr TStringBuf TANKER_VOL_TYPE_RUB = "1";
constexpr TStringBuf TANKER_VOL_TYPE_TANK = "3";
// Attentions
constexpr TStringBuf NO_GAS_STATION = "no_gas_station";
constexpr TStringBuf NOT_AUTHORIZED = "not_authorized";
constexpr TStringBuf NO_CREDIT_CARD = "no_credit_card";
constexpr TStringBuf INVALID_VOLUME = "invalid_volume";
constexpr TStringBuf INVALID_FUEL = "invalid_fuel";
constexpr TStringBuf INVALID_COLUMN = "invalid_column";
constexpr TStringBuf FULL_TANK_NOT_SUPPORTED = "full_tank_not_supported";

constexpr double DEFAULT_MONEY_THRESHOLD = 99;

void AddColumnSuggest(NBASS::TContext& ctx, i32 column) {
    NSc::TValue suggest;
    suggest["value"] = column;

    NSc::TValue formUpdate;
    formUpdate["name"] = NAVI_REFUEL_FORM_NAME;
    formUpdate["resubmit"].SetBool(true);
    NSc::TValue& slot = formUpdate["slots"].SetArray().Push();
    slot["name"].SetString(SLOT_COLUMN_NAME);
    slot["type"].SetString(SLOT_COLUMN_TYPE);
    slot["optional"].SetBool(true);
    slot["value"] = column;

    ctx.AddSuggest("refuel__column",  std::move(suggest), std::move(formUpdate));
}

void AddFuelSuggest(NBASS::TContext& ctx, const TFuel& fuel) {
    TString fuelSuggest = fuel.FuelType;
    if (!fuel.Name.empty()) {
        fuelSuggest += " ";
        fuelSuggest += fuel.Name;
    }
    NSc::TValue suggest;
    suggest["value"] = std::move(fuelSuggest);

    NSc::TValue formUpdate;
    formUpdate["name"] = NAVI_REFUEL_FORM_NAME;
    formUpdate["resubmit"].SetBool(true);
    NSc::TValue& slot = formUpdate["slots"].SetArray().Push();
    slot["name"].SetString(SLOT_FUEL_TYPE_NAME);
    slot["type"].SetString(SLOT_FUEL_TYPE_TYPE);
    slot["optional"].SetBool(true);
    slot["value"] = fuel.FuelId;

    ctx.AddSuggest("refuel__fuel", std::move(suggest), std::move(formUpdate));
}

void AddVolumeSuggest(NBASS::TContext& ctx, TStringBuf volType, int vol) {
    NSc::TValue suggest;
    suggest["volume"] = vol;
    suggest["volume_type"] = volType;


    NSc::TValue formUpdate;
    formUpdate["name"] = NAVI_REFUEL_FORM_NAME;
    formUpdate["resubmit"].SetBool(true);
    NSc::TValue& slot = formUpdate["slots"].SetArray().Push();
    slot["name"].SetString(SLOT_VOL_NAME);
    slot["type"].SetString(SLOT_VOL_TYPE);
    slot["optional"].SetBool(true);
    slot["value"] = vol;

    NSc::TValue& typeSlot = formUpdate["slots"].SetArray().Push();
    typeSlot["name"].SetString(SLOT_VOL_TYPE_NAME);
    typeSlot["type"].SetString(SLOT_VOL_TYPE_TYPE);
    typeSlot["optional"].SetBool(true);
    typeSlot["value"] = volType;

    ctx.AddSuggest("refuel__volume", std::move(suggest), std::move(formUpdate));
}

void AddCancelSuggest(NBASS::TContext& ctx) {
    NSc::TValue suggest;
    suggest["value"] = TStringBuf("Закончить");
    ctx.AddSuggest("refuel__cancel", std::move(suggest));
}

void InvalidateRequiredSlot(TSlot* slot) {
    slot->Reset();
    slot->Optional = false;
}
bool ValidateBounds(const TSlot& slot, double low, double hi) {
    const double vol = slot.Value.GetNumber();
    return (vol >= low && vol <= hi);
}

bool ValidateVolume(const TSlot& volSlot, const TSlot& volTypeSlot, const TTankerInfo& tankerInfo) {
    TStringBuf volType = volTypeSlot.Value.GetString();
    if (SLOT_VOL_TYPE_LITER == volType && tankerInfo.VolumeRange) {
        const auto minVol = tankerInfo.VolumeRange->Min;
        const auto maxVol =  tankerInfo.VolumeRange->Max;
        return ValidateBounds(volSlot, minVol, maxVol);
    } else if (SLOT_VOL_TYPE_RUB == volType && tankerInfo.MoneyRange) {
        const auto minSum = tankerInfo.MoneyRange->Min;
        const auto maxSum =  tankerInfo.MoneyRange->Max;
        return ValidateBounds(volSlot, minSum, maxSum);
    }
    return true;
}

bool ValidateColumn(const TSlot& colSlot, const TGasStation& stationInfo) {
    const auto col = colSlot.Value.GetIntNumber();
    return !(stationInfo.find(col) == stationInfo.end());
}

bool ValidateFuelType(const TSlot& colSlot, TStringBuf fuelId, const TGasStation& stationInfo) {
    const auto col = colSlot.Value.GetIntNumber();

    const auto& fuels = stationInfo.at(col);
    return AnyOf(fuels, [&](const auto& fuel) { return fuel.FuelId == fuelId; });
}

double GetDefaultMoneyThreshold(const TTankerInfo &tankerInfo) {
    if (tankerInfo.VolumeRange) {
        return tankerInfo.VolumeRange->Max;
    }
    return DEFAULT_MONEY_THRESHOLD;
}

} // namespace

TResultValue TNavigatorRefuelHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::NAVI_COMMANDS);
    TMaybe<TTankerInfo> tankerInfo;
    // Check if user has gas station nearby.
    if (ctx.Meta().HasLocation())
        tankerInfo = FetchTankerInfo(ctx);
    if (tankerInfo.Empty()) {
        ctx.AddAttention(NO_GAS_STATION);
        return TResultValue();
    }
    // If user is unauthorized, we can't continue.
    if (!ctx.IsAuthorizedUser()) {
        constexpr TStringBuf navigatorIntent = "yandex-auth://";
        NSc::TValue intentData;
        intentData["uri"].SetString(navigatorIntent);
        ctx.AddCommand<TNavigatorAuth4RefuelDirective>(TStringBuf("open_uri"), intentData);

        ctx.AddAttention(NOT_AUTHORIZED);

        return TResultValue();
    }

    const auto& stationInfo = tankerInfo->GasStation;
    const auto& userFuelId = tankerInfo->UserFuelId;
    bool hasPaymentMethod = tankerInfo->HasPaymentMethod;
    // Fill slots one by one (unless provided).
    bool stopHere = false;
    TSlot* columnSlot = ctx.GetOrCreateSlot(SLOT_COLUMN_NAME, SLOT_COLUMN_TYPE);
    if (IsSlotEmpty(columnSlot)) {
        columnSlot->Optional = false;
        stopHere = true;
    } else if (!ValidateColumn(*columnSlot, stationInfo)) {
        ctx.AddAttention(INVALID_COLUMN);
        InvalidateRequiredSlot(columnSlot);
        stopHere = true;
    }

    if (stopHere) {
        for (const auto& kvp : stationInfo)
            AddColumnSuggest(ctx, kvp.first);
        AddCancelSuggest(ctx);
        return TResultValue();
    }

    TSlot* fuelSlot = ctx.GetOrCreateSlot(SLOT_FUEL_TYPE_NAME, SLOT_FUEL_TYPE_TYPE);
    if (IsSlotEmpty(fuelSlot)) {
        fuelSlot->Optional = false;
        if (userFuelId.empty()) {
            // No fuel data - ask user.
            stopHere = true;
        } else {
            // Try to fill Fuel slot with user preferences.
            stopHere = !ValidateFuelType(*columnSlot, TStringBuf(userFuelId), stationInfo);
            if (!stopHere) {
                fuelSlot->Value.SetString(userFuelId);
            }
        }
    } else if (!ValidateFuelType(*columnSlot, fuelSlot->Value.GetString(), stationInfo)) {
        ctx.AddAttention(INVALID_FUEL);
        InvalidateRequiredSlot(fuelSlot);
        stopHere = true;
    }

    if (stopHere) {
        const auto column = columnSlot->Value.GetIntNumber();
        auto it = stationInfo.find(column);
        if (it != stationInfo.end()) {
            for (const auto& fuel : it->second)
                AddFuelSuggest(ctx, fuel);
        }
        AddCancelSuggest(ctx);
        return TResultValue();
    }

    TSlot* volSlot = ctx.GetOrCreateSlot(SLOT_VOL_NAME, SLOT_VOL_TYPE);
    TSlot* volTypeSlot = ctx.GetOrCreateSlot(SLOT_VOL_TYPE_NAME, SLOT_VOL_TYPE_TYPE);
    if (!IsSlotEmpty(volSlot) && volSlot->Value.GetNumber() == 0) {
        int tryNormalize = NMarket::NormalizeAmount(volSlot->Value.GetString());
        if (tryNormalize) {
            volSlot->Value = tryNormalize;
        }
    }

    if (!IsSlotEmpty(volSlot) && IsSlotEmpty(volTypeSlot)) {
        volTypeSlot->Value = volSlot->Value.GetNumber() > GetDefaultMoneyThreshold(tankerInfo.GetRef()) ?
                                SLOT_VOL_TYPE_RUB : SLOT_VOL_TYPE_LITER;
    }

    const bool volFilled = !(IsSlotEmpty(volSlot) || IsSlotEmpty(volTypeSlot));
    const bool volIsValid = volFilled && ValidateVolume(*volSlot, *volTypeSlot, tankerInfo.GetRef());
    // Full tank is not supported now.
    const bool isFullTank = volTypeSlot->Value.GetString() == SLOT_VOL_TYPE_TANK;
    if (!volIsValid || isFullTank) {
        AddVolumeSuggest(ctx, SLOT_VOL_TYPE_RUB, 500);
        AddVolumeSuggest(ctx, SLOT_VOL_TYPE_RUB, 1000);
        AddVolumeSuggest(ctx, SLOT_VOL_TYPE_LITER, 10);
        // Uncomment when full tank will be supported.
        // AddVolumeSuggest(ctx, SLOT_VOL_TYPE_TANK, 1);
        AddCancelSuggest(ctx);
        if (isFullTank)
            ctx.AddAttention(FULL_TANK_NOT_SUPPORTED);
        else if (volFilled)
            ctx.AddAttention(INVALID_VOLUME);
        InvalidateRequiredSlot(volSlot);
        return TResultValue();
    }

    // If user attached credit card and confirmed - go to payment straight away.
    // Otherwise (user didn't confirm or no payment info attached) - open Benzin UI.
    TSlot* confirmationSlot = ctx.GetOrCreateSlot(SLOT_CONFIRMATION_NAME, SLOT_CONFIRMATION_TYPE);
    if (hasPaymentMethod) {
        if (IsSlotEmpty(confirmationSlot)) {
            confirmationSlot->Optional = false;

            ctx.AddSuggest(TStringBuf("refuel__confirm"));
            ctx.AddSuggest(TStringBuf("refuel__decline"));

            // Add station name as slot.
            if (!tankerInfo->GasStationName.empty())
                ctx.CreateSlot(SLOT_STATION_NAME_NAME, SLOT_STATION_NAME_TYPE, true, TStringBuf(tankerInfo->GasStationName));

            return TResultValue();
        }
    } else {
        ctx.AddAttention(NO_CREDIT_CARD); // No card - no confirmation, just send link.
    }

    // Fill url for tanker.
    TCgiParameters params;
    params.InsertUnescaped(TStringBuf("id"), tankerInfo->GasStationId);
    params.InsertUnescaped(TStringBuf("column_id"), columnSlot->Value.ForceString());
    params.InsertUnescaped(TStringBuf("fuel_id"), fuelSlot->Value.ForceString());
    auto volType = volTypeSlot->Value.ForceString();
    // volume types encoding  : rub=1, liter=2, tank=3
    if (SLOT_VOL_TYPE_RUB == volType) {
        params.InsertUnescaped(TStringBuf("volume"), volSlot->Value.ForceString());
        params.InsertUnescaped(TStringBuf("volume_type"), TANKER_VOL_TYPE_RUB);
    } else if (SLOT_VOL_TYPE_LITER == volType) {
        params.InsertUnescaped(TStringBuf("volume"), volSlot->Value.ForceString());
        params.InsertUnescaped(TStringBuf("volume_type"), TANKER_VOL_TYPE_LITER);
    } else if (SLOT_VOL_TYPE_TANK == volType) {
        constexpr TStringBuf DUMMY_VOL = "1";
        params.InsertUnescaped(TStringBuf("volume"), DUMMY_VOL);
        params.InsertUnescaped(TStringBuf("volume_type"), TANKER_VOL_TYPE_TANK);
    }
    if (hasPaymentMethod && confirmationSlot->Value.GetString() == SLOT_CONF_YES)
        params.InsertUnescaped(TStringBuf("confirmation"), TStringBuf("1"));
    TRefuelNavigatorIntent navigatorIntent = TRefuelNavigatorIntent(ctx, params);
    return navigatorIntent.Do();
}

void TNavigatorRefuelHandler::Register(THandlersMap* handlers) {
    auto cbNavigatorRefuelForm = []() {
        return MakeHolder<TNavigatorRefuelHandler>();
    };

    handlers->RegisterFormHandler(NAVI_REFUEL_FORM_NAME, cbNavigatorRefuelForm);
    handlers->RegisterFormHandler(NAVI_REFUEL_ELLIPSIS_FORM_NAME, cbNavigatorRefuelForm);
}

}
