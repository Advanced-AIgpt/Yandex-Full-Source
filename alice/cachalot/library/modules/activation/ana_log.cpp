#include <alice/cachalot/library/modules/activation/ana_log.h>


namespace NCachalot {

    constexpr TStringBuf ITEM_TYPE_ACTIVATION_LOG = "activation_log";
    constexpr TStringBuf ITEM_TYPE_ACTIVATION_LOG_FINAL = "activation_log_final";


    void TSpotterValidationDevice::SetNobody() {
        if (Status < ESVDS_Nobody) {
            Status = ESVDS_Nobody;
        }
    }

    void TSpotterValidationDevice::SetLeader() {
        if (Status < ESVDS_Leader) {
            Status = ESVDS_Leader;
        }
    }

    void TSpotterValidationDevice::SetCertain(TString deviceId) {
        if (Status < ESVDS_Certain) {
            Status = ESVDS_Certain;
            DeviceId = std::move(deviceId);
        }
    }

    bool TSpotterValidationDevice::Update(const TSpotterValidationDevice& other) {
        if (Status < other.Status) {
            Status = other.Status;
            DeviceId = other.DeviceId;
            return true;
        }
        return false;
    }

    bool TSpotterValidationDevice::IsUnknown() const {
        return ESVDS_Unknown == Status;
    }

    bool TSpotterValidationDevice::IsValid() const {
        return ESVDS_Nobody < Status;
    }

    bool TSpotterValidationDevice::DeviceIdIsReal() const {
        return ESVDS_Certain <= Status;
    }

    TString TSpotterValidationDevice::GetDeviceId() const {
        switch (Status) {
            case ESVDS_Unknown:
                return UNKNOWN_DEVICE_ID;
            case ESVDS_Nobody:
                return NOBODY_DEVICE_ID;
            case ESVDS_Leader:
                return "Leader";
            case ESVDS_Certain:
                return DeviceId;
            default:
                Y_ASSERT(false);
                return UNKNOWN_DEVICE_ID;
        }
    }


    TMaybe<TString> TActivationAnaLog::UpdateSpotterValidatedBy(const TSpotterValidationDevice& svd) {
        SpotterValidationDevice.Update(svd);
        if (SpotterValidationDevice.IsValid()) {
            return SpotterValidationDevice.GetDeviceId();
        }
        return Nothing();
    }

    void TActivationAnaLog::FinalizeSpotterValidatedBy(bool final) {
        if (!SpotterValidationDevice.IsUnknown()) {
            if (SpotterValidationDevice.IsValid()) {
                MutableProto()->SetThisSpotterIsValid(true);

                // non-real device_id values are not logged on first&second stage,
                // so LoadActivationLog never reads non-real values.
                if (SpotterValidationDevice.DeviceIdIsReal() || final) {
                    MutableProto()->SetSpotterValidatedBy(SpotterValidationDevice.GetDeviceId());
                }
            } else {
                // it is known that spotter is not valid
                MutableProto()->SetThisSpotterIsValid(false);
            }
        } else {
            // keep fields untouched
        }
    }

    bool TActivationAnaLog::LoadActivationLog(NAppHost::TServiceContextPtr ctx) {
        if (ctx->HasProtobufItem(ITEM_TYPE_ACTIVATION_LOG)) {
            ActivationLog = ctx->GetOnlyProtobufItem<NCachalotProtocol::TActivationLog>(ITEM_TYPE_ACTIVATION_LOG);

            if (ActivationLog.HasSpotterValidatedBy()) {
                // non-real device_id values are not logged on first&second stage,
                // thus here we have some real device_id.
                Y_ASSERT(ActivationLog.GetThisSpotterIsValid());
                SpotterValidationDevice.SetCertain(ActivationLog.GetSpotterValidatedBy());
            }

            return true;
        }
        return false;
    }

    void TActivationAnaLog::StoreActivationLog(NAppHost::TServiceContextPtr ctx, bool final) {
        FinalizeSpotterValidatedBy(final);
        ctx->AddProtobufItem(ActivationLog, final ? ITEM_TYPE_ACTIVATION_LOG_FINAL : ITEM_TYPE_ACTIVATION_LOG);
    }

    NCachalotProtocol::TActivationLog* TActivationAnaLog::MutableProto() {
        return &ActivationLog;
    }


}  // namespace NCachalot
