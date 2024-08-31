#include "converter.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/maybe.h>


namespace {

    // Default value. See "ignore rms" logic in cachalot/activation.
    static constexpr double DEFAULT_AVG_RMS = 0.0;


    class TRmsCoefficientStorage {
    public:
        explicit TRmsCoefficientStorage(const TString& rmsPerDeviceConfig) {
            NJson::TJsonValue jsonMap;
            NJson::ReadJsonTree(rmsPerDeviceConfig, &jsonMap, /* throwOnError = */ true);
            for (const auto& [model, value] : jsonMap.GetMap()) {
                Y_ASSERT(value.IsDouble());
                DeviceModel2Coefficient[model] = value.GetDouble();
            }
        }

        TRmsCoefficientStorage()
            : TRmsCoefficientStorage(NResource::Find("/per_device_model_rms_correction_coefficients.json"))
        {
        }

        TMaybe<double> TryGetCorrectionForModel(const TString& deviceModel) const {
            if (auto iter = DeviceModel2Coefficient.find(deviceModel); iter != DeviceModel2Coefficient.end()) {
                return iter->second;
            }
            return Nothing();
        }

        static const TRmsCoefficientStorage& GetInstance() {
            return *Singleton<TRmsCoefficientStorage>();
        }

    private:
        THashMap<TString, double> DeviceModel2Coefficient;
    };


    double AverageRMS(const NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeaturesV0& features) {
        double sum = 0.0;
        unsigned counter = 0;
        for (const auto& data : features.GetRmsData()) {
            double sum2 = 0.0;
            unsigned counter2 = 0;
            for (auto value : data.GetValues()) {
                sum2 += value;
                counter2++;
            }
            if (counter2) {
                sum += sum2 / counter2;
            }
            counter++;
        }
        return counter ? sum / counter : DEFAULT_AVG_RMS;
    }

    double AverageRMS(const NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeaturesV1& features) {
        double avg = 0;
        if (features.GetVersion() == 1) {
            double sum = 0.0;
            unsigned counter = 0;
            for (const auto& [k, data] : features.GetRmsData()) {
                double sum2 = 0.0;
                unsigned counter2 = 0;
                for (auto value : data.GetValues()) {
                    sum2 += value;
                    counter2++;
                }
                if (counter2) {
                    sum += sum2 / counter2;
                }
                counter++;
            }
            avg = counter ? sum / counter : DEFAULT_AVG_RMS;
        }
        return avg / 100;
    }

    double CorrectRawAvgRmsForModel(
        double rawAvgRMS,
        const TString& deviceModel,
        const TString& rmsPerDeviceConfig
    ) {
        const TRmsCoefficientStorage& rmsCoefficientStorage = rmsPerDeviceConfig.empty()
            ? TRmsCoefficientStorage::GetInstance()
            : TRmsCoefficientStorage(rmsPerDeviceConfig);

        const TMaybe<double> coef = rmsCoefficientStorage.TryGetCorrectionForModel(deviceModel);
        if (coef.Defined()) {
            return coef.GetRef() * rawAvgRMS;
        }
        return rawAvgRMS;
    }

    void ParseRmsData(
        const NJson::TJsonValue::TArray& arr,
        NAliceProtocol::TRequestContext::TAdditionalOptions::TChannelRmsData* data
    ) {
        Y_ASSERT(data != nullptr);

        for (const NJson::TJsonValue& number : arr) {
            if (number.IsInteger()) {
                data->AddValues(number.GetInteger());
            } else if (number.IsUInteger()) {
                data->AddValues(number.GetUInteger());
            } else if (number.IsDouble()) {
                data->AddValues(number.GetDouble());
            }
        }
    }

    NAliceProtocol::TRequestContext::TAdditionalOptions::TChannelRmsData ParseRmsData(
        const NJson::TJsonValue::TArray& arr
    ) {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TChannelRmsData data;
        ParseRmsData(arr, &data);
        return data;
    }

}  // anonymous

double NAlice::NCuttlefish::NAppHostServices::NRmsConverter::CalcAvgRMS(
    const NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures& features,
    const TString& deviceModel,
    const TString& rmsPerDeviceConfig
) {
    if (features.HasVer1()) {
        if (features.GetVer1().HasRawAvgRMS()) {
            return CorrectRawAvgRmsForModel(features.GetVer1().GetRawAvgRMS(), deviceModel, rmsPerDeviceConfig);
        }
        if (features.GetVer1().HasAvgRMS()) {
            return features.GetVer1().GetAvgRMS();
        }
        return AverageRMS(features.GetVer1());
    }
    if (features.HasVer0()) {
        return AverageRMS(features.GetVer0());
    }
    return DEFAULT_AVG_RMS;
}

void NAlice::NCuttlefish::NAppHostServices::NRmsConverter::ConvertSpotterFeatures(
    const NJson::TJsonValue& spotterRms,
    NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures* output
) {
    Y_ENSURE(output != nullptr);

    if (spotterRms.IsArray()) {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeaturesV0* features = output->MutableVer0();
        for (const auto& channel : spotterRms.GetArray()) {
            if (channel.IsArray()) {
                ParseRmsData(channel.GetArray(), features->AddRmsData());
            }
        }
    } else if (spotterRms.IsMap()) {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeaturesV1* features = output->MutableVer1();
        if (const auto* avgRMS = spotterRms.GetValueByPath("AvgRMS"); avgRMS && avgRMS->IsDouble()) {
            features->SetAvgRMS(avgRMS->GetDouble());
        }
        if (const auto* rawAvgRMS = spotterRms.GetValueByPath("RawAvgRMS"); rawAvgRMS && rawAvgRMS->IsDouble()) {
            features->SetRawAvgRMS(rawAvgRMS->GetDouble());
        }
        if (const auto* version = spotterRms.GetValueByPath("version"); version && version->IsInteger()) {
            features->SetVersion(version->GetInteger());
        }
        if (const auto* channels = spotterRms.GetValueByPath("channels"); channels && channels->IsArray()) {
            for (const auto& channel : channels->GetArray()) {
                if (channel.IsMap() &&
                    channel.Has("name") && channel["name"].IsString() &&
                    channel.Has("data") && channel["data"].IsArray()
                ) {
                    features->MutableRmsData()->insert({
                        channel["name"].GetString(),
                        ParseRmsData(channel["data"].GetArray())
                    });
                }
            }
        }
    }
}
