#pragma once

#include "blackbox_api.h"
#include "data_sync_api.h"

#include <alice/bass/forms/context/context.h>

#include <alice/library/passport_api/passport_api.h>
#include <alice/library/biometry/biometry.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NBASS {

class TBiometryDelegate final : public NAlice::NBiometry::TBiometry::IDelegate {
public:
    TBiometryDelegate(TContext& ctx, TBlackBoxAPI& blackBoxAPI, TDataSyncAPI& dataSyncAPI)
        : Ctx(ctx)
        , BlackBoxAPI(blackBoxAPI)
        , DataSyncAPI(dataSyncAPI) {
    }

    TMaybe<TString> GetUid() const override {
        TString uid;
        if (BlackBoxAPI.GetUid(Ctx, uid)) {
            return uid;
        }
        return {};
    }

    NAlice::NBiometry::TResult GetUserName(TStringBuf UserId, TString& userName) const override {
        if (const auto error = DataSyncAPI.Get(Ctx, UserId, TPersonalDataHelper::EUserSpecificKey::UserName, userName)) {
            return NAlice::NBiometry::TError{NAlice::NBiometry::TError::EType::DataSync, error->Msg};
        }
        return {};
    }

    NAlice::NBiometry::TResult GetGuestId(TStringBuf UserId, TString& guestId) const override {
        if (const auto error = DataSyncAPI.Get(Ctx, UserId, TPersonalDataHelper::EUserSpecificKey::GuestUID, guestId)) {
            return NAlice::NBiometry::TError{NAlice::NBiometry::TError::EType::DataSync, error->Msg};
        }
        return {};
    }

private:
    TContext& Ctx;
    TBlackBoxAPI& BlackBoxAPI;
    TDataSyncAPI& DataSyncAPI;
};

} // namespace NBASS
