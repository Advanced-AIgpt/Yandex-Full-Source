#pragma once

#include <alice/hollywood/library/scenarios/market/common/types.h>

#include <library/cpp/json/json_value.h>

namespace NAlice::NHollywood::NMarket {

class TPicture {
public:
    TPicture();
    explicit TPicture(const NJson::TJsonValue& data);

    const TString& GetUrl() const { return Url; }
    ui64 GetHeight() const { return Height; }
    ui64 GetWidth() const { return Width; }
    ui64 GetContainerHeight() const { return ContainerHeight; }
    ui64 GetContainerWidth() const { return ContainerWidth; }
    double GetOriginalRatio() const { return OriginalRatio; }
    void SetOriginalRatio(double ratio) { OriginalRatio = ratio; }
    NJson::TJsonValue ToJson() const;

    static TPicture GetMostSuitablePicture(
        const NJson::TJsonValue::TArray& pictures,
        const TCgiGlFilters& glFilters = TCgiGlFilters());

private:
    TString Url;
    ui64 Height;
    ui64 Width;
    ui64 ContainerHeight;
    ui64 ContainerWidth;
    double OriginalRatio;

    static TMaybe<NJson::TJsonValue> GetMostSuitableThumb(const NJson::TJsonValue::TArray& thumbs);
    static TMaybe<NJson::TJsonValue> GetPictureByGlFilters(
        const TCgiGlFilters& glFilters,
        const NJson::TJsonValue::TArray& pictures);
    static size_t GetFittingFiltersCount(
        const TCgiGlFilters& glFilters,
        const NJson::TJsonValue& picture);
};

} // namespace NAlice::NHollywood::NMarket
