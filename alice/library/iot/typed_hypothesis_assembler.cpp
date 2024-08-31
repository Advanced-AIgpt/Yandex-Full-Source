#include "typed_hypothesis_assembler.h"

#include "defs.h"
#include "structs.h"

#include <util/generic/algorithm.h>


namespace NAlice {

using namespace NScenarios;
using TDateTime = TBegemotIotNluResult::THypothesis::TDateTime;

namespace {

class TTypedHypothesisAssembler {
public:
    TTypedHypothesisAssembler(const NSc::TValue& input, TBegemotIotNluResult::THypothesis& result)
            : Result(result)
            , Input(input)
            , ErrorMessage{}
    {
    }

    TIotParseErrorMessage Assemble() {
        ErrorMessage = Nothing();
        SetContent();
        SetDatetimeRange();
        SetDevicesNames();
        SetRoomsNames();
        SetHouseholds();
        SetGroupsNames();
        SetRawEntities();
        SetNlg();
        SetId();
        return ErrorMessage;
    }

private:
    TBegemotIotNluResult::THypothesis& Result;
    const NSc::TValue& Input;
    TIotParseErrorMessage ErrorMessage;

private:
    void SetContent() {
        if (Input.Has("request_type") && Input.Get("request_type").GetString() == "query") {
            SetQuery();
        } else {
            SetAction();
        }
    }

    void SetAction() {
        auto& action = *Result.MutableAction();
        const auto& inputAction = Input["action"];

        if (inputAction.Has("type")) {
            action.SetType(inputAction.Get("type").ForceString());
        }
        if (inputAction.Has("instance")) {
            action.SetInstance(inputAction.Get("instance").ForceString());
        }
        if (inputAction.Has("relative")) {
            action.SetRelative(inputAction.Get("relative").ForceString());
        }
        if (inputAction.Has("unit")) {
            action.SetUnit(inputAction.Get("unit").ForceString());
        }

        if (inputAction.Has("value")) {
            switch (inputAction.Get("value").GetType()) {
                case NSc::TValue::EType::String:
                    action.SetStrValue(inputAction.Get("value").ForceString());
                    break;
                case NSc::TValue::EType::IntNumber:
                    action.SetIntValue(inputAction.Get("value").ForceIntNumber());
                    break;
                case NSc::TValue::EType::Bool:
                    action.SetBoolValue(inputAction.Get("value").GetBool());
                    break;
                default:
                    ErrorMessage = "bad action value type";
            }
        }
    }

    void SetQuery() {
        auto& query = *Result.MutableQuery();
        const auto& inputQuery = Input["action"];

        if (inputQuery.Has("type")) {
            query.SetType(inputQuery.Get("type").ForceString());
        }
        if (inputQuery.Has("instance")) {
            query.SetInstance(inputQuery.Get("instance").ForceString());
        }
        if (inputQuery.Has("target")) {
            query.SetTarget(inputQuery.Get("target").ForceString());
        }
    }

    void SetDatetimeRange() {
        if (!Input.Has("datetime")) {
            return;
        }

        const auto& datetime = Input.Get("datetime");

        if (datetime.Has("end") != datetime.Has("start")) {
            ErrorMessage = "bad datetime field: Has(end) != Has(start)";
            return;
        }

        if (datetime.Has("start")) {
            SetDatetime(datetime.Get("start"), Result.MutableDateTimeRange()->MutableStart());
            SetDatetime(datetime.Get("end"), Result.MutableDateTimeRange()->MutableEnd());
        } else {
            SetDatetime(datetime, Result.MutableConcreteDateTime());
        }
    }

    void AssembleTimeUnits(const NSc::TValue& datetime, const TString& fieldName, TDateTime* outputDateTime,
                           TDateTime::TTimeUnits* (TDateTime::*mutableTimeUnitsGetter)()) {
        if (!datetime.Has(fieldName)) {
            return;
        }

        TDateTime::TTimeUnits* output = (outputDateTime->*mutableTimeUnitsGetter)();
        Y_ASSERT(output != nullptr);

        output->SetValue(datetime.Get(fieldName).ForceIntNumber());

        const auto isRelativeFieldName = fieldName + "_relative";
        if (datetime.Has(isRelativeFieldName) && datetime.Get(isRelativeFieldName).GetBool()) {
            output->SetIsRelative(true);
        }
    }

    void SetDatetime(const NSc::TValue& datetime, TDateTime* output) {
        AssembleTimeUnits(datetime, "years", output, &TDateTime::MutableYears);
        AssembleTimeUnits(datetime, "months", output, &TDateTime::MutableMonths);
        AssembleTimeUnits(datetime, "days", output, &TDateTime::MutableDays);
        AssembleTimeUnits(datetime, "hours", output, &TDateTime::MutableHours);
        AssembleTimeUnits(datetime, "minutes", output, &TDateTime::MutableMinutes);
        AssembleTimeUnits(datetime, "seconds", output, &TDateTime::MutableSeconds);

        if (datetime.Has("weekday")) {
            const auto weekDay = datetime.Get("weekday").ForceIntNumber();
            output->SetWeekday(static_cast<TDateTime::EWeekday>(weekDay));
        }
        if (datetime.Has("period")) {
            const auto period = datetime.Get("period").ForceString();
            auto typedPeriod = TDateTime::P_NONE;
            if (period == "am") {
                typedPeriod = TDateTime::P_AM;
            } else if (period == "pm") {
                typedPeriod = TDateTime::P_PM;
            }
            output->SetPeriod(typedPeriod);
        }
    }

    void SetDevicesNames() {
        for (const auto& deviceId : Input.Get("devices").GetArray()) {
            *Result.MutableDevicesIds()->Add() = deviceId.ForceString();
        }
    }

    void SetRoomsNames() {
        for (const auto& roomId : Input.Get("rooms").GetArray()) {
            *Result.MutableRoomsIds()->Add() = roomId.ForceString();
        }
    }

    void SetHouseholds() {
        for (const auto& householdId : Input.Get("households").GetArray()) {
            *Result.MutableHouseholdIds()->Add() = householdId.ForceString();
        }
    }

    void SetGroupsNames() {
        for (const auto& groupId : Input.Get("groups").GetArray()) {
            *Result.MutableGroupsIds()->Add() = groupId.ForceString();
        }
    }

    void SetRawEntities() {
        for (const auto& rawEntity : Input.Get("raw_entities").GetArray()) {
            *Result.AddRawEntities() = NIot::TRawEntity::FromValue(rawEntity).AsEntity();
        }
    }

    void SetNlg() {
        for (const auto& nlgVariant : Input.Get("nlg").Get("variants").GetArray()) {
            *Result.MutableNlg()->AddVariants() = nlgVariant.ForceString();
        }
    }

    void SetId() {
        Result.SetId(static_cast<int>(Input.Get("id").GetIntNumber()));
    }
};

}  // namespace

TIotParseErrorMessage AssembleTypedHypothesis(const NSc::TValue& input, TBegemotIotNluResult::THypothesis& result) {
    return TTypedHypothesisAssembler(input, result).Assemble();
}

}  // namespace NAlice
