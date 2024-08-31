#include "geocoder.h"

#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/common/personal_data.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/source_request/source_request.h>

#include <alice/bass/util/error.h>

#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>

#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

namespace {

constexpr TStringBuf GEOSEARCH_ORIGIN = "assistant_bass";

NHttpFetcher::TFetchStatus ParseGeoCoderResponse(
    TStringBuf responseData,
    TStringBuf text,
    TResultValue& result,
    TString* roadName)
{
    NSc::TValue json = NSc::TValue::FromJson(responseData);
    const NSc::TValue& found = json.TrySelect("features");
    if (found.IsNull()) {
        result = TError(
            TError::EType::NOGEOFOUND,
            TStringBuilder() << "cannot parse num of results for text: " << text
        );
        return NHttpFetcher::TFetchStatus::Failure("noGeoFound");
    }

    size_t resultsCount = found.GetArray().size();
    for (size_t i = 0; i < resultsCount; ++i) {
        NSc::TValue kind = found.TrySelect(
            TStringBuilder() << i << "/properties/GeocoderMetaData/kind"
        );
        NSc::TValue name = found.TrySelect(
            TStringBuilder() << i << "/properties/name"
        );

        if (!kind.IsNull() && !name.IsNull() && kind.ForceString("") == "street") {
            *roadName = name.ForceString("");
            return NHttpFetcher::TFetchStatus::Success();
        }
    }
    result = TError(
        TError::EType::NOGEOFOUND,
        TStringBuilder() << "no road found for text: " << text
    );
    return NHttpFetcher::TFetchStatus::Failure("noGeoFound");
}

} // anonymous namespace

TResultValue LLToGeo(const TContext& ctx, double lat, double lon, NGeobase::TId* userId) {
    Y_ASSERT(userId);

    const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
    NGeobase::TId regionId = geobase.GetRegionIdByLocation(lat, lon);
    if (!NAlice::IsValidId(regionId)) {
        const TString errMsg = TStringBuilder() << "Unable to find region for lat: " << lat << ", lon: " << lon;
        LOG(ERR) << errMsg << Endl;
        return TError{TError::EType::SYSTEM, errMsg};
    }

    // may be geoid is bigger then city or don't have city parent
    NGeobase::TId cityRegion = geobase.GetParentIdWithType(regionId, static_cast<int>(NGeobase::ERegionType::CITY));
    LOG(DEBUG) << "Found region: " << regionId << " for lat, lon: " << lat << ", " << lon
               << "; reduced to city level region: " << cityRegion << Endl;
    *userId = NAlice::IsValidId(cityRegion) ? cityRegion : regionId;

    return Nothing();
}

TResultValue TextToRoadName(TContext& ctx, TStringBuf text, TString* roadName) {
    Y_ASSERT(roadName);
    TResultValue result;

    THolder<NHttpFetcher::TRequest> request = ctx.GetSources().GeoCoderTextToRoadName().Request(
        [&result, &text, roadName] (TStringBuf responseData) {
            return ParseGeoCoderResponse(responseData, text, result, roadName);
    });

    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("origin"), GEOSEARCH_ORIGIN);
    cgi.InsertUnescaped(TStringBuf("text"), text);
    cgi.InsertUnescaped(TStringBuf("type"), TStringBuf("geo"));
    if (!ctx.Meta().Location().IsNull()) {
        double lat = ctx.Meta().Location().Lat();
        double lon = ctx.Meta().Location().Lon();
        cgi.InsertUnescaped(TStringBuf("ll"), TStringBuilder() << lon << ',' << lat);
    }
    cgi.InsertUnescaped(TStringBuf("ms"), TStringBuf("json"));
    cgi.InsertUnescaped(TStringBuf("lang"), ctx.MetaLocale().ToString());
    request->AddCgiParams(cgi);

    NHttpFetcher::TResponse::TRef resp = request->Fetch()->Wait();
    if (resp->IsError() && !result) {
        TStringBuilder errText;
        errText << TStringBuf("Fetching from geocoder error: ") << resp->GetErrorText();
        LOG(DEBUG) << errText << Endl;
        return TError(
            TError::EType::SYSTEM,
            errText
        );
    }
    return result;

}

}
