#pragma once

#include <alice/bass/forms/vins.h>
#include <alice/bass/libs/smallgeo/region.h>
#include <alice/bass/util/error.h>

namespace NBASS {

class TRequestedGeo;

class TWeatherFormHandler: public IHandler {
public:
    struct TErrorWrapper {
        TErrorWrapper(TError::EType type, TStringBuf msg, bool critical = true)
            : Error(type, msg)
            , Critical(critical)
        {
        }

        TError Error;
        bool Critical;
    };
    using TErrorStatus = TMaybe<TErrorWrapper>;

public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);

    /** Prepare query and request weather API (for automative only).
     * It sets critial to the specific error in if something critical is happened and always return false.
     * If false is returned and critical is not changed then no weather found for the given region.
     * @param[in] ctx is a context
     * @param[in] requestedCity is a city requested by user or determined by position or from vins
     * @param[out] weatherJson is a json returned from the weather API (or isn't changed in case of error)
     * @param[out] responseGeo is a geo object which is corresponded with the answer of weather API, could be different from requested one (pointer could be null)
     * @param[out] critial is a result which is set to the proper TError block in case the error is critical (pointer could be null)
     * @param[in] currentPosition is a flag which tells to request weather API by coordinates instead of geo
     * @param[in] position is exact coordinates to request weather. 'requestedCity' should be valid anyway
     * @return true if everything is ok otherwise false
     */
    static TErrorStatus RequestWeather(TContext& ctx, const TRequestedGeo& requestedCity, NSc::TValue* weatherJson,
                                       TMaybe<NGeobase::TRegion>& responseGeo, bool currentPosition = false,
                                       TMaybe<NSmallGeo::TLatLon> position = Nothing());
};

} // namespace NBASS
