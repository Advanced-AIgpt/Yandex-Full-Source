#pragma once

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/source_request/source_request.h>

namespace NBASS::NMusic {

struct TStation {
    TString Name;
    TString Tag;
};

class TStationsData {
public:
    explicit TStationsData(const IGlobalContext& globalCtx)
        : YaRadioBackgroundSF(globalCtx.Sources().YaRadioBackground, globalCtx.Config(), TStringBuf("/stations/list"),
                              SourcesRegistryDelegate)
    {
        Update();
    }

    const TStation& GetStation(TStringBuf stationTag) const {
        const TStation* found = Stations.FindPtr(stationTag);
        return found ? *found : Default<TStation>();
    }

private:
    void Update();

private:
    using TStations = THashMap<TString, TStation>;

    TDummySourcesRegistryDelegate SourcesRegistryDelegate;
    TSourceRequestFactory YaRadioBackgroundSF;
    THashMap<TString, TStation> Stations;
};

} // namespace NBASS::NMusic
