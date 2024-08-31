#include "fm_radio.h"

#include <alice/megamind/protos/common/location.pb.h>

namespace NAlice::NHollywood::NMusic {

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/automotive/fm_radio.cpp?rev=r8680832#L15
i32 ChooseSupportedRegionId(
    const TScenarioRunRequestWrapper& request,
    const TFmRadioResources& fmRadioResources)
{
    if (request.BaseRequestProto().HasLocation()) {
        return fmRadioResources.GetNearest(request.BaseRequestProto().GetLocation().GetLat(), request.BaseRequestProto().GetLocation().GetLon());
    }
    return 0;
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/automotive/fm_radio.cpp?rev=r8680832#L46
i32 GetRegionId(
    const TScenarioRunRequestWrapper& request,
    const TFmRadioResources& fmRadioResources)
{
    /* const auto& deviceState = request.BaseRequestProto().GetDeviceState();
    if (deviceState.HasFmRadio() && deviceState.GetFmRadio().HasRegionId()
        && fmRadioResources.HasRegion(deviceState.GetFmRadio().GetRegionId()))
    {
        return deviceState.GetFmRadio().GetRegionId();
    }*/

    return ChooseSupportedRegionId(request, fmRadioResources);
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/radio.cpp?rev=r8680832#L110
TMaybe<TString> GetFmRadioByFreq(
    const TScenarioRunRequestWrapper& request,
    const double radioFreq,
    const TFmRadioResources& fmRadioResources)
{
    i32 regionId = GetRegionId(request, fmRadioResources);

    if (regionId == 0) {
        return Nothing();
    }
    TString radioFreqTrunc = ToString(trunc(radioFreq * 100));
    if (fmRadioResources.HasFmRadioByRegion(regionId, radioFreqTrunc)) {
        return fmRadioResources.GetFmRadioByRegion(regionId, radioFreqTrunc);
    }
    return Nothing();
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/radio.cpp?rev=r8680832#L643
TMaybe<TString> GetFmRadioName(
    const TScenarioRunRequestWrapper& request,
    bool isFmRadio,
    const TPtrWrapper<TSlot>& slotFmRadio,
    const TPtrWrapper<TSlot>& slotFmRadioFreq,
    const TFmRadioResources& fmRadioResources)
{
    return isFmRadio
        ? slotFmRadio->Value.AsString()
        : GetFmRadioByFreq(request, slotFmRadioFreq->Value.As<double>().GetRef(), fmRadioResources);
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/radio.cpp?rev=r8793832#L649
TMaybe<TString> GetFmRadioId(
    const TMaybe<TString>& namedFmRadioStation,
    const TFmRadioResources& fmRadioResources)
{
    if (!namedFmRadioStation) {
        return Nothing();
    }

    if (const auto* fmRadioIdPtr = fmRadioResources.GetNameToFmRadioId().FindPtr(*namedFmRadioStation.Get())) {
        return *fmRadioIdPtr;
    }
    return Nothing();
}

} // namespace NAlice::NHollywood::NMusic
