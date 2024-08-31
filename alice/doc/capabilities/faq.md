# Часто задаваемые вопросы

### Я разрабатываю сценарий в Hollywood. Как понять, поддерживает ли запрос нужное мне умение? 
В Голливуде уже написаны обёртки, которые позволяют найти Endpoint и распарсить Capability. 
Пример из сценария скринсейвера: 
```cpp
    inline TString GetDeviceIdOfTvDevice(const TRunRequest& request, const TDeviceState& deviceState) {
        if (deviceState.GetTandemState().GetConnected()) {
            auto envHelper = NHollywood::TEnvironmentStateHelper(request);
            return envHelper.GetTandemFollowerApplication().GetDeviceId();
        }

        return request.Client().GetClientInfo().DeviceId;
    }

    inline bool SupportsScreensaver(const TRunRequest& request, const NAlice::TDeviceState& deviceState) {
        if (const NScenarios::TDataSource* dataSource = request.GetDataSource(EDataSourceType::ENVIRONMENT_STATE)) {
            const NAlice::TEnvironmentState* envState = &dataSource->GetEnvironmentState();

            const NAlice::TEndpoint* endpoint = NHollywood::FindEndpoint(*envState, GetDeviceIdOfTvDevice(request, deviceState));
            if (!endpoint) {
                return false;
            }

            TScreensaverCapability capability;
            return NHollywood::ParseTypedCapability(capability, *endpoint);
        }
        return false;
    }
```
