#pragma once

#include <alice/bass/forms/market/types.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS {

namespace NMarket {

class TPicture {
public:
    TPicture();
    explicit TPicture(const NSc::TValue& data);

    const TString& GetUrl() const { return Url; }
    ui64 GetHeight() const { return Height; }
    ui64 GetWidth() const { return Width; }
    ui64 GetContainerHeight() const { return ContainerHeight; }
    ui64 GetContainerWidth() const { return ContainerWidth; }
    double GetOriginalRatio() const { return OriginalRatio; }
    void SetOriginalRatio(double ratio) { OriginalRatio = ratio; }

    static TPicture GetMostSuitablePicture(const NSc::TValue& data, const TCgiGlFilters& glFilters = TCgiGlFilters());

private:
    TString Url;
    ui64 Height;
    ui64 Width;
    ui64 ContainerHeight;
    ui64 ContainerWidth;
    double OriginalRatio;

    static TMaybe<NSc::TValue> GetMostSuitableThumb(const NSc::TArray& thumbs);
    static TMaybe<NSc::TValue> GetPictureByGlFilters(const TCgiGlFilters& glFilters, const NSc::TArray& pictures);
    static size_t GetFittingFiltersCount(const TCgiGlFilters& glFilters, const NSc::TValue& picture);
};

} // namespace NMarket

} // namespace NBASS
