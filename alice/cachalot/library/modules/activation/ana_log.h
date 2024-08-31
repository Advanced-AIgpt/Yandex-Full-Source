#pragma once

#include <alice/cachalot/api/protos/cachalot.pb.h>

#include <apphost/api/service/cpp/service_context.h>


namespace NCachalot {

    static const TString NOBODY_DEVICE_ID = "Nobody!";
    static const TString UNKNOWN_DEVICE_ID = "Unknown";

    class TSpotterValidationDevice {
    private:
        enum ESpotterValidationDeviceStatus {
            ESVDS_Unknown = 0,
            ESVDS_Nobody = 1,
            ESVDS_Leader = 2,
            ESVDS_Certain = 3,
        };

    public:
        void SetNobody();
        void SetLeader();
        void SetCertain(TString deviceId);
        bool Update(const TSpotterValidationDevice& other);

        bool IsUnknown() const;
        bool IsValid() const;
        bool DeviceIdIsReal() const;
        TString GetDeviceId() const;

    private:
        ESpotterValidationDeviceStatus Status = ESVDS_Unknown;
        TString DeviceId;
    };


    class TActivationAnaLog {
    public:
        TMaybe<TString> UpdateSpotterValidatedBy(const TSpotterValidationDevice& svd);

        bool LoadActivationLog(NAppHost::TServiceContextPtr ctx);
        void StoreActivationLog(NAppHost::TServiceContextPtr ctx, bool final);

        NCachalotProtocol::TActivationLog* MutableProto();

    private:
        void FinalizeSpotterValidatedBy(bool final);

    private:
        TSpotterValidationDevice SpotterValidationDevice;
        NCachalotProtocol::TActivationLog ActivationLog;
    };

}  // namespace NCachalot
