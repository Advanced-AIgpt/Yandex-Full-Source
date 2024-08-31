#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>

#include <library/cpp/json/writer/json_value.h>


namespace NAlice::NCuttlefish::NAppHostServices::NRmsConverter {

    double CalcAvgRMS(
        const NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures& features,
        const TString& deviceModel,
        const TString& rmsPerDeviceConfig = ""
    );

    void ConvertSpotterFeatures(
        const NJson::TJsonValue& spotterRms,
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures* output
    );

}  // namespace NAlice::NCuttlefish::NRmsConverter
