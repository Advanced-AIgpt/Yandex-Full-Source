#include "maps_static_api.h"

#include <util/string/builder.h>
#include <util/system/env.h>

namespace NBASS {

namespace NMapsStaticApi {
TImageUrlBuilder::TImageUrlBuilder(TContext& ctx) : MapStaticApiUrl{ctx.GetConfig().StaticMapApi().Url().Get()} {
    Params.InsertUnescaped(TStringBuf("l"), TStringBuf("map"));
    Params.InsertUnescaped(TStringBuf("lang"), ctx.MetaLocale().ToString());
    Params.InsertUnescaped(TStringBuf("lg"), TStringBuf("0"));
    Params.InsertUnescaped(TStringBuf("cr"), TStringBuf("0"));
    Params.InsertUnescaped(TStringBuf("key"), ctx.GetConfig().StaticMapApi().Key().Get());
}

TImageUrlBuilder& TImageUrlBuilder::Set(const TStringBuf param, const TStringBuf value) {
    Params.ReplaceUnescaped(param, value);
    return *this;
}

TImageUrlBuilder& TImageUrlBuilder::SetSize(ui16 width, ui16 height) {
    TStringBuilder value;
    value << width << ',' << height;
    Params.InsertEscaped(TStringBuf("size"), value);
    return *this;
}

TImageUrlBuilder& TImageUrlBuilder::SetCenter(float lon, float lat) {
    TStringBuilder value;
    value << lon << ',' << lat;
    Params.InsertEscaped(TStringBuf("ll"), value);
    return *this;
}

TImageUrlBuilder& TImageUrlBuilder::SetZoom(ui16 zoom) {
    Params.InsertEscaped(TStringBuf("z"), ToString(zoom));
    return *this;
}

TString TImageUrlBuilder::Build() {
    TStringBuilder url;
    if (!Points.Empty()) {
        Params.ReplaceUnescaped(PointsParamName, Points);
    }
    url << MapStaticApiUrl << '?' << Params.Print();
    return url;
}

TImageUrlBuilder& TImageUrlBuilder::AddPoint(float lon, float lat, const TStringBuf marker) {
    if (!Points.Empty()) {
        Points << '~';
    }
    Points << lon << ',' << lat << ',' << marker;
    return *this;
}

} // namespace NMapsStaticApiÏ

} // namespace NBASS
