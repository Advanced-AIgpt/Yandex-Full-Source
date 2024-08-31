#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/resources/geobase.h>
#include <alice/hollywood/library/resources/common_resources.h>
#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>
#include <alice/hollywood/library/scenarios/get_date/get_date_frames.h>

#include <alice/megamind/protos/common/frame.pb.h>

#include <alice/library/geo/geodb.h>
#include <alice/library/sys_datetime/sys_datetime.h>

namespace NAlice::NHollywoodFw::NGetDate {


enum class ETargetQuestion {
    DayOfWeek,   // В ответе ожидается день недели
    Date,        // В ответе ожидается дата (день, месяц)
    All,         // В ответе ожидается и то, и другое
    Week,        // В ответе ожидается номер недели
    Month,       // В ответе ожидается номер и название месяца
    Year,        // В ответе ожидается год (по факту рендерим полностью)
    AllWithYear, // То же, что и All, но с обязательным произношением года
    Default,     // В ответе нет запроса таргета
    Unknown
};

/*
    Convert TENSE info
*/
inline NAlice::TSysDatetimeParser::ETense FillTenseInfo(const TStringBuf verb) {
    if (verb == "past") {
        return NAlice::TSysDatetimeParser::ETense::TensePast;
    }
    if (verb == "future") {
        return NAlice::TSysDatetimeParser::ETense::TenseFuture;
    }
    // For all other cases - use present, NLG can be filled later
    return NAlice::TSysDatetimeParser::ETense::TenseDefault;
}

/*
    Get target question info
*/
inline ETargetQuestion GetTargetQuestionInfo(const TGetDateSceneArgs& args) {
    const auto verb = args.GetQueryTarget();
    if (verb == "date") {
        return ETargetQuestion::Date;
    }
    if (verb == "day_of_week") {
        return ETargetQuestion::DayOfWeek;
    }
    if (verb == "date_and_day_of_week" ) {
        return ETargetQuestion::All;
    }
    if (verb == "week" ) {
        return ETargetQuestion::Week;
    }
    if (verb == "month" ) {
        return ETargetQuestion::Month;
    }
    if (verb == "year" ) {
        return ETargetQuestion::Year;
    }
    if (verb == "all_with_year" ) {
        return ETargetQuestion::AllWithYear;
    }
    if (verb == "") {
        return ETargetQuestion::Default;
    }
    // Undefined target, must be validated in main function
    return ETargetQuestion::Unknown;
}

/*
    Get target question info from sys_datetime
*/
inline ETargetQuestion ConvertSysDatetimeTypeToDirectQuestion(TSysDatetimeParser::EDatetimeParser parseResult) {
    switch (parseResult) {
        case TSysDatetimeParser::EDatetimeParser::Unknown:
            return ETargetQuestion::Unknown;
        case TSysDatetimeParser::EDatetimeParser::Fixed:
        case TSysDatetimeParser::EDatetimeParser::RelativeFuture:
        case TSysDatetimeParser::EDatetimeParser::RelativePast:
        case TSysDatetimeParser::EDatetimeParser::RelativeMix:
            return ETargetQuestion::All;
        case TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek:
            return ETargetQuestion::DayOfWeek;
        case TSysDatetimeParser::EDatetimeParser::Mix:
            return ETargetQuestion::All;
    }
}

inline ETargetQuestion ConvertSysDatetimeTypeToReverseQuestion(TSysDatetimeParser::EDatetimeParser parseResult) {
    switch (parseResult) {
        case TSysDatetimeParser::EDatetimeParser::Unknown:
            return ETargetQuestion::Unknown;
        case TSysDatetimeParser::EDatetimeParser::Fixed:
        case TSysDatetimeParser::EDatetimeParser::RelativeFuture:
        case TSysDatetimeParser::EDatetimeParser::RelativePast:
        case TSysDatetimeParser::EDatetimeParser::RelativeMix:
            return ETargetQuestion::DayOfWeek;
        case TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek:
            return ETargetQuestion::All;
        case TSysDatetimeParser::EDatetimeParser::Mix:
            return ETargetQuestion::All;
    }
}

/*
    Get geo location info
    return false, if error
*/
inline bool GetLocationInfo(const TRunRequest& runRequest, const TGetDateSceneArgs& args, TGetDateRenderProto& renderProto, TString& tz) {
    renderProto.SetResultCity("");
    const TString slotValue = args.GetWhere();
    const TString slotType = args.GetWhereType();
    if (slotValue.Empty() || slotType.Empty()) {
        return true;
    }

    NGeobase::TId id;
    TString regionName;
    const TGeoUtilStatus errorStatus = (slotType == SLOT_WHERE_SLOT_TYPE_ADDRESS) ? ParseGeoAddrAddress(slotValue, id) : ParseSysGeo(slotValue, id);
    if (errorStatus.Empty() && NAlice::IsValidId(id)) {
        const auto& geobase = runRequest.System().GetCommonResources().Resource<NAlice::NHollywood::TGeobaseResource>().GeobaseLookup();
        regionName = geobase.GetRegionById(id).GetName();
        const auto tzNew = geobase.GetRegionById(id).GetTimezoneName();

        if (tzNew != "") {
            tz = tzNew;
        }
        renderProto.SetResultCity(regionName);
        renderProto.SetGeoId(id);

        NSc::TValue caseForms;
        NAlice::AddAllCaseForms(geobase, id, TStringBuf("ru"), &caseForms, /* wantObsolete = */ true);
        renderProto.SetCityPreparse(caseForms.ToJsonSafe());
        renderProto.SetIsCustomCity(true);
        return true;
    }
    return false;
}

inline bool FillCurrentLocationInfo(const TRunRequest& runRequest, TGetDateRenderProto& renderProto, TString& tz) {
    renderProto.SetResultCity("");
    const TUserLocation userLocation = runRequest.GetUserLocation();
    TString regionName;

    if (!NAlice::IsValidId(userLocation.UserRegion())) {
        LOG_ERROR(runRequest.Debug().Logger()) << "Can't detect user region";
        return false;
    }

    const auto& geobase = runRequest.System().GetCommonResources().Resource<NAlice::NHollywood::TGeobaseResource>().GeobaseLookup();
    regionName = geobase.GetRegionById(userLocation.UserRegion()).GetName();
    const auto tzNew = geobase.GetRegionById(userLocation.UserRegion()).GetTimezoneName();

    tz = tzNew;
    renderProto.SetResultCity(regionName);
    renderProto.SetGeoId(userLocation.UserRegion());

    NSc::TValue caseForms;
    NAlice::AddAllCaseForms(geobase, userLocation.UserRegion(), TStringBuf("ru"), &caseForms, /* wantObsolete = */ true);
    renderProto.SetCityPreparse(caseForms.ToJsonSafe());
    renderProto.SetIsCustomCity(false);
    return true;
}

}  // namespace NAlice::NHollywood::NGetDate
