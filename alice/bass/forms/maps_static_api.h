#pragma once

#include <alice/bass/forms/context/context.h>

#include <library/cpp/scheme/scheme.h>
#include <util/generic/strbuf.h>
#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

namespace NMapsStaticApi {

class TImageUrlBuilder {
public:
    TImageUrlBuilder(TContext& ctx);
    TImageUrlBuilder& Set(const TStringBuf param, const TStringBuf value);
    TImageUrlBuilder& SetSize(ui16 width, ui16 height);
    TImageUrlBuilder& SetCenter(float lon, float lat);
    TImageUrlBuilder& AddPoint(float lon, float lat, const TStringBuf marker);
    TImageUrlBuilder& SetZoom(ui16 zoom);
    TString Build();
private:
    TString MapStaticApiUrl;
    TStringBuilder Points;
    TCgiParameters Params;

    static constexpr TStringBuf PointsParamName = "pt";
};

} // namespace NMapsStaticApi

} // namespace NBASS
